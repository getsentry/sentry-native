import { test } from 'node:test';
import assert from 'node:assert/strict';
import { publishWardenRisk, rateCheckRun } from './warden-pr-risk.ts';

const HEAD = 'dc24ceecbc725e2547e9df19ff0e42c9277b06b3';

// Shape of a real `warden: sdk-pr-risk` check summary (getsentry/sentry-native#2335).
const MEDIUM_SUMMARY = [
  'sdk-pr-risk: Found 1 issue (1 medium)',
  '',
  '### Medium',
  '',
  '<details>',
  '<summary><strong>Enables new Warden skill via build-release config (build-release)</strong> - `warden.toml:77-98`</summary>',
  '',
  'warden.toml is a build-release path so this hunk cannot be low.',
  '',
  '</details>',
  '',
  '---',
  '<sub>⏱ 23.2s · 30.8k in / 2.1k out · $0.07</sub>',
].join('\n');

function checkRun(overrides: Record<string, unknown> = {}) {
  return {
    id: 1,
    status: 'completed',
    conclusion: 'neutral',
    html_url: 'https://github.com/o/r/runs/1',
    output: { title: '1 issue', summary: MEDIUM_SUMMARY },
    ...overrides,
  };
}

function fakeGitHub({
  checkRuns = [[checkRun()]] as unknown[][],
  labels = [] as string[],
  comments = [] as unknown[],
  labelExists = true,
} = {}) {
  const calls: [string, Record<string, unknown>][] = [];
  const record = (name: string, result: unknown = {}) => async (params: Record<string, unknown>) => {
    calls.push([name, params]);
    return result;
  };
  let poll = 0;
  const listComments = Symbol('listComments');
  const github = {
    paginate: async (method: unknown, params: Record<string, unknown>) => {
      assert.equal(method, listComments);
      calls.push(['listComments', params]);
      return comments;
    },
    rest: {
      checks: {
        listForRef: async (params: Record<string, unknown>) => {
          calls.push(['listForRef', params]);
          return { data: { check_runs: checkRuns[Math.min(poll++, checkRuns.length - 1)] } };
        },
      },
      issues: {
        listLabelsOnIssue: record('listLabelsOnIssue', { data: labels.map((name) => ({ name })) }),
        removeLabel: record('removeLabel'),
        addLabels: record('addLabels'),
        getLabel: async (params: Record<string, unknown>) => {
          calls.push(['getLabel', params]);
          if (!labelExists) throw Object.assign(new Error('Not Found'), { status: 404 });
          return {};
        },
        createLabel: record('createLabel'),
        listComments,
        createComment: record('createComment'),
        updateComment: record('updateComment'),
      },
    },
  };
  return { github, calls };
}

const context = { repo: { owner: 'o', repo: 'r' }, payload: { pull_request: { number: 7, head: { sha: HEAD } } } };
const core = { info: () => {}, summary: { addRaw: () => ({ write: async () => {} }) } };
const run = (github: unknown) =>
  publishWardenRisk({ github: github as never, context, core, sleep: async () => {}, pollMs: 1, timeoutMs: 3 });
const names = (calls: [string, unknown][]) => calls.map(([name]) => name);

test('rates a PR by its highest finding severity', () => {
  const rating = rateCheckRun(checkRun());
  assert.equal(rating?.level, 'medium');
  assert.deepEqual(rating?.reasons, ['Enables new Warden skill via build-release config (build-release)']);
});

test('rates a PR without findings as low', () => {
  const rating = rateCheckRun(checkRun({ output: { title: 'No issues', summary: 'sdk-pr-risk: No issues found\n\nNo issues found.' } }));
  assert.equal(rating?.level, 'low');
  assert.deepEqual(rating?.reasons, []);
});

test('does not rate a skipped check', () => {
  assert.equal(rateCheckRun(checkRun({ output: { title: 'Skipped', summary: 'Trigger did not run for this event.' } })), undefined);
});

test('first run creates the label and the comment', async () => {
  const { github, calls } = fakeGitHub({ labelExists: false });
  const rating = await run(github);
  assert.equal(rating?.level, 'medium');
  assert.deepEqual(names(calls), ['listLabelsOnIssue', 'listForRef', 'getLabel', 'createLabel', 'addLabels', 'listComments', 'createComment']);
  assert.deepEqual(calls[3][1], { owner: 'o', repo: 'r', name: 'warden risk: medium', color: 'FBCA04', description: 'Warden PR risk: medium' });
  assert.deepEqual(calls[4][1], { owner: 'o', repo: 'r', issue_number: 7, labels: ['warden risk: medium'] });
  const body = String(calls[6][1].body);
  assert.match(body, /^<!-- warden-pr-risk -->\n### PR risk \(Warden\): medium\n/);
  assert.match(body, /For commit `dc24cee`/);
});

test('a new push replaces the old label and updates the existing comment', async () => {
  const { github, calls } = fakeGitHub({
    labels: ['warden risk: high', 'bug'],
    comments: [
      { id: 1, body: '<!-- warden-pr-risk -->\nforged', user: { login: 'someone' } },
      { id: 2, body: '<!-- warden-pr-risk -->\nold', user: { login: 'github-actions[bot]' } },
    ],
  });
  await run(github);
  assert.deepEqual(calls.find(([name]) => name === 'removeLabel')?.[1], { owner: 'o', repo: 'r', issue_number: 7, name: 'warden risk: high' });
  assert.equal(calls.filter(([name]) => name === 'removeLabel').length, 1);
  assert.equal(calls.find(([name]) => name === 'updateComment')?.[1].comment_id, 2);
  assert.ok(!names(calls).includes('createComment'));
});

test('waits for the check to complete', async () => {
  const { github, calls } = fakeGitHub({ checkRuns: [[], [checkRun({ status: 'in_progress' })], [checkRun()]] });
  assert.equal((await run(github))?.level, 'medium');
  assert.equal(calls.filter(([name]) => name === 'listForRef').length, 3);
});

test('a PR without a rating gets no label and no new comment', async () => {
  const { github, calls } = fakeGitHub({ checkRuns: [[]] });
  assert.equal(await run(github), undefined);
  // Polls at 0, 1, 2 and 3 ms, then gives up at the 3 ms timeout.
  assert.deepEqual(names(calls), ['listLabelsOnIssue', 'listForRef', 'listForRef', 'listForRef', 'listForRef', 'listComments']);
});

test('a PR that loses its rating has its comment rewritten', async () => {
  const { github, calls } = fakeGitHub({
    checkRuns: [[checkRun({ output: { title: 'Skipped', summary: '' } })]],
    comments: [{ id: 2, body: '<!-- warden-pr-risk -->\nold', user: { login: 'github-actions[bot]' } }],
  });
  await run(github);
  assert.match(String(calls.find(([name]) => name === 'updateComment')?.[1].body), /not rated/);
  assert.ok(!names(calls).includes('addLabels'));
});

test('rethrows label errors other than 404', async () => {
  const { github } = fakeGitHub();
  github.rest.issues.getLabel = async () => {
    throw Object.assign(new Error('boom'), { status: 500 });
  };
  await assert.rejects(run(github), /boom/);
});
