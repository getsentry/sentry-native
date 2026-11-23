// Publishes the Warden `sdk-pr-risk` rating as one PR comment and one label,
// the way getsentry/classifiers publishes its risk label. Warden itself only
// reports the rating in a check run, so this reads that check run.

const CHECK_NAME = 'warden: sdk-pr-risk';
const MARKER = '<!-- warden-pr-risk -->';
const COMMENT_AUTHOR = 'github-actions[bot]';
const MAX_REASONS = 5;

const LEVELS = ['high', 'medium', 'low'] as const;
type Level = (typeof LEVELS)[number];

const LABEL_COLORS: Record<Level, string> = { high: 'B60205', medium: 'FBCA04', low: '0E8A16' };
const labelName = (level: Level): string => `warden risk: ${level}`;
const LABELS = LEVELS.map(labelName);

interface CheckRun {
  id: number;
  status: string;
  conclusion: string | null;
  html_url: string | null;
  output: { title: string | null; summary: string | null };
}

interface Comment {
  id: number;
  body?: string;
  user: { login: string } | null;
}

type Params = Record<string, unknown>;

interface GitHub {
  paginate<T>(method: unknown, params: Params): Promise<T[]>;
  rest: {
    checks: {
      listForRef(params: Params): Promise<{ data: { check_runs: CheckRun[] } }>;
    };
    issues: {
      listLabelsOnIssue(params: Params): Promise<{ data: { name: string }[] }>;
      removeLabel(params: Params): Promise<unknown>;
      addLabels(params: Params): Promise<unknown>;
      getLabel(params: Params): Promise<unknown>;
      createLabel(params: Params): Promise<unknown>;
      listComments: unknown;
      createComment(params: Params): Promise<unknown>;
      updateComment(params: Params): Promise<unknown>;
    };
  };
}

interface Context {
  repo: { owner: string; repo: string };
  payload: { pull_request?: { number: number; head: { sha: string } } };
}

interface Core {
  info(message: string): void;
  summary: { addRaw(text: string): { write(): Promise<unknown> } };
}

export interface Rating {
  level: Level;
  reasons: string[];
  checkUrl: string | null;
}

/** Reads the PR risk from a completed `sdk-pr-risk` check: the highest finding severity, or low when there are none. */
export function rateCheckRun(check: CheckRun): Rating | undefined {
  if (check.status !== 'completed' || check.output.title === 'Skipped') return undefined;
  const summary = check.output.summary ?? '';
  // Warden groups findings under `### High`, `### Medium`, `### Low`, highest first.
  const heading = summary.match(/^### (High|Medium|Low)$/m);
  const level = heading ? (heading[1].toLowerCase() as Level) : 'low';
  const reasons = [...summary.matchAll(/<summary><strong>(.+?)<\/strong>/g)]
    .map((match) => match[1])
    .slice(0, MAX_REASONS);
  return { level, reasons, checkUrl: check.html_url };
}

/** Renders the PR comment for a rating, or for a PR that was not rated. */
function renderComment(headSha: string, rating: Rating | undefined): string {
  const commit = `For commit \`${headSha.slice(0, 7)}\`.`;
  if (!rating) {
    return [MARKER, '### PR risk (Warden): not rated', '', commit, '', `The \`${CHECK_NAME}\` check did not produce a rating.`].join('\n');
  }
  const source = rating.checkUrl ? `[\`${CHECK_NAME}\`](${rating.checkUrl})` : `\`${CHECK_NAME}\``;
  const lines = [MARKER, `### PR risk (Warden): ${rating.level}`, '', `${commit} From ${source}.`, ''];
  if (rating.reasons.length > 0) {
    lines.push('Raised by:', ...rating.reasons.map((reason) => `- ${reason}`), '');
  } else {
    lines.push('No changed hunk was rated above low.', '');
  }
  lines.push('Shadow mode, for comparison with getsentry/classifiers. This label is a hint for reviewers, not an approval.');
  return lines.join('\n');
}

const defaultSleep = (ms: number): Promise<void> => new Promise((resolve) => setTimeout(resolve, ms));

function isStatus(error: unknown, status: number): boolean {
  return typeof error === 'object' && error !== null && (error as { status?: number }).status === status;
}

/** Waits for the Warden check on the PR head, clears the old rating, then publishes the new label and comment. */
export async function publishWardenRisk({
  github,
  context,
  core,
  sleep = defaultSleep,
  pollMs = 15_000,
  timeoutMs = 15 * 60_000,
}: {
  github: GitHub;
  context: Context;
  core: Core;
  sleep?: (ms: number) => Promise<void>;
  pollMs?: number;
  timeoutMs?: number;
}): Promise<Rating | undefined> {
  const pr = context.payload.pull_request;
  if (!pr) throw new Error('This script needs a pull_request event.');
  const { owner, repo } = context.repo;
  const issue = { owner, repo, issue_number: pr.number };
  const headSha = pr.head.sha;

  // Clear first, so a rating for an older commit never sits on a new one.
  const { data: current } = await github.rest.issues.listLabelsOnIssue(issue);
  for (const { name } of current.filter((label) => LABELS.includes(label.name))) {
    try {
      await github.rest.issues.removeLabel({ ...issue, name });
    } catch (error) {
      if (!isStatus(error, 404)) throw error;
    }
  }

  let rating: Rating | undefined;
  for (let waited = 0; ; waited += pollMs) {
    const { data } = await github.rest.checks.listForRef({ owner, repo, ref: headSha, check_name: CHECK_NAME, filter: 'latest' });
    const check = data.check_runs.find((run) => run.status === 'completed');
    if (check) {
      rating = rateCheckRun(check);
      break;
    }
    if (waited >= timeoutMs) break;
    core.info(`Waiting for ${CHECK_NAME} on ${headSha}`);
    await sleep(pollMs);
  }

  if (rating) {
    const name = labelName(rating.level);
    try {
      await github.rest.issues.getLabel({ owner, repo, name });
    } catch (error) {
      if (!isStatus(error, 404)) throw error;
      await github.rest.issues.createLabel({ owner, repo, name, color: LABEL_COLORS[rating.level], description: `Warden PR risk: ${rating.level}` });
    }
    await github.rest.issues.addLabels({ ...issue, labels: [name] });
  }

  const body = renderComment(headSha, rating);
  const comments = await github.paginate<Comment>(github.rest.issues.listComments, { ...issue, per_page: 100 });
  const existing = comments.find((comment) => comment.user?.login === COMMENT_AUTHOR && comment.body?.startsWith(MARKER));
  if (existing) {
    await github.rest.issues.updateComment({ owner, repo, comment_id: existing.id, body });
  } else if (rating) {
    // A PR that was never rated gets no "not rated" comment.
    await github.rest.issues.createComment({ ...issue, body });
  }

  await core.summary.addRaw(body).write();
  return rating;
}
