#!/usr/bin/env python3

import argparse
import re
from pathlib import Path


def unique_contributors(contributors):
    unique = {}
    for contributor in contributors:
        contributor = contributor.lstrip("@")
        unique.setdefault(contributor.lower(), contributor)
    return list(unique.values())


def update_changelog(contents, release, contributors):
    heading = re.search(rf"^## {re.escape(release)}$", contents, re.MULTILINE)
    if not heading:
        print(f"Release {release} not found in the changelog")
        return contents

    end = contents.find("\n## ", heading.end())
    if end == -1:
        end = len(contents)

    section = contents[heading.start() : end]
    thanks = "**Thank you**:"
    thanks_start = section.find(thanks)
    existing = set()
    if thanks_start != -1:
        existing = {
            contributor.lower()
            for contributor in re.findall(
                r"https://github\.com/([A-Za-z0-9-]+)", section[thanks_start:]
            )
        }

    contributors = [
        contributor
        for contributor in unique_contributors(contributors)
        if contributor.lower() not in existing
    ]
    if not contributors:
        print(f"All contributors are already credited in {release}")
        return contents

    entries = "\n".join(
        f"- [{contributor}](https://github.com/{contributor})"
        for contributor in contributors
    )
    if thanks_start != -1:
        section = f"{section.rstrip()}\n{entries}\n"
    else:
        section = f"{section.rstrip()}\n\n{thanks}\n\n{entries}\n"

    credited = ", ".join(f"@{contributor}" for contributor in contributors)
    print(f"Credited {credited} in {release}")
    return contents[: heading.start()] + section + contents[end:]


def parse_args():
    parser = argparse.ArgumentParser(description="Credit contributors in CHANGELOG.md")
    parser.add_argument("contributors", nargs="+")
    parser.add_argument("--release", default="Unreleased")
    return parser.parse_args()


def main():
    args = parse_args()
    changelog = Path(__file__).resolve().parents[1] / "CHANGELOG.md"
    contents = changelog.read_text()
    updated = update_changelog(contents, args.release, args.contributors)
    if contents != updated:
        changelog.write_text(updated)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
