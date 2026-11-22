#!/usr/bin/env python3
"""Plot the duration of the last N completed runs of every workflow in a GitHub repo.

Usage:
    export GITHUB_TOKEN=ghp_xxx        # or just be logged in with `gh auth login`
    pip install requests matplotlib
    python gh_workflow_durations.py OWNER/REPO [--limit 100] [--branch main] [-w 'CI*'] [--output durations.png]
"""
import argparse
import fnmatch
import os
import subprocess
import sys
from datetime import datetime

import matplotlib.pyplot as plt
import requests

API = "https://api.github.com"


def get_token():
    token = os.environ.get("GITHUB_TOKEN") or os.environ.get("GH_TOKEN")
    if token:
        return token
    try:
        return subprocess.check_output(["gh", "auth", "token"], text=True).strip()
    except Exception:
        return None


def parse(ts):
    return datetime.strptime(ts, "%Y-%m-%dT%H:%M:%SZ")


def get_workflows(session, repo):
    workflows, page = [], 1
    while True:
        r = session.get(f"{API}/repos/{repo}/actions/workflows",
                        params={"per_page": 100, "page": page})
        r.raise_for_status()
        batch = r.json()["workflows"]
        workflows += batch
        if len(batch) < 100:
            return workflows
        page += 1


def get_runs(session, repo, workflow_id, limit, branch):
    runs, page = [], 1
    while len(runs) < limit:
        params = {"per_page": min(100, limit), "page": page, "status": "completed"}
        if branch:
            params["branch"] = branch
        r = session.get(f"{API}/repos/{repo}/actions/workflows/{workflow_id}/runs",
                        params=params)
        r.raise_for_status()
        batch = r.json()["workflow_runs"]
        runs += batch
        if len(batch) < params["per_page"]:
            break
        page += 1
    return runs[:limit]


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("repo", help="OWNER/REPO")
    ap.add_argument("--limit", type=int, default=100, help="runs per workflow (default 100)")
    ap.add_argument("--branch", help="only runs on this branch")
    ap.add_argument("-w", "--workflow", action="append", metavar="GLOB",
                    help="only workflows whose name matches this glob, case-insensitive "
                         "(repeatable), e.g. -w 'CI*' -w '*lint*'")
    ap.add_argument("--output", default="workflow_durations.png")
    ap.add_argument("--no-show", action="store_true", help="save only, don't open a window")
    args = ap.parse_args()

    session = requests.Session()
    session.headers["Accept"] = "application/vnd.github+json"
    token = get_token()
    if token:
        session.headers["Authorization"] = f"Bearer {token}"
    else:
        print("Warning: no token found, you will hit rate limits quickly.", file=sys.stderr)

    data = {}  # workflow name -> list of (start, minutes, conclusion)
    workflows = get_workflows(session, args.repo)
    if args.workflow:
        patterns = [g.lower() for g in args.workflow]
        workflows = [wf for wf in workflows
                     if any(fnmatch.fnmatchcase(wf["name"].lower(), g) for g in patterns)]
        if not workflows:
            sys.exit(f"No workflow name matches {args.workflow}")

    for wf in workflows:
        runs = get_runs(session, args.repo, wf["id"], args.limit, args.branch)
        points = []
        for run in runs:
            if run["conclusion"] == "skipped" or not run.get("run_started_at"):
                continue
            start = parse(run["run_started_at"])
            minutes = (parse(run["updated_at"]) - start).total_seconds() / 60
            points.append((start, minutes, run["conclusion"]))
        if points:
            points.sort(key=lambda p: p[0])
            data[wf["name"]] = points
            mins = [p[1] for p in points]
            print(f"{wf['name']:40s} runs={len(mins):3d}  "
                  f"avg={sum(mins)/len(mins):6.1f} min  max={max(mins):6.1f} min")

    if not data:
        sys.exit("No completed runs found.")

    fig, (ax1, ax2) = plt.subplots(
        2, 1, figsize=(13, 9), gridspec_kw={"height_ratios": [3, 2]})

    # Timeline: one line per workflow, failed runs marked with a red x
    for name, pts in data.items():
        xs = [p[0] for p in pts]
        ys = [p[1] for p in pts]
        line, = ax1.plot(xs, ys, marker="o", markersize=3, linewidth=1, label=name)
        fails = [(p[0], p[1]) for p in pts if p[2] == "failure"]
        if fails:
            ax1.scatter(*zip(*fails), marker="x", color="red", s=40, zorder=3)
    ax1.set_title(f"{args.repo}: duration of last {args.limit} runs per workflow "
                  "(red x = failed)")
    ax1.set_ylabel("Duration (minutes)")
    ax1.grid(alpha=0.3)
    ax1.legend(fontsize=8, loc="upper left", bbox_to_anchor=(1.01, 1))

    # Distribution per workflow
    names = list(data)
    ax2.boxplot([[p[1] for p in data[n]] for n in names], showfliers=True)
    ax2.set_xticks(range(1, len(names) + 1))
    ax2.set_xticklabels(names)
    ax2.set_ylabel("Duration (minutes)")
    ax2.set_title("Distribution per workflow")
    ax2.grid(alpha=0.3, axis="y")
    plt.setp(ax2.get_xticklabels(), rotation=30, ha="right", fontsize=8)

    fig.tight_layout()
    fig.savefig(args.output, dpi=150, bbox_inches="tight")
    print(f"Saved {args.output}")
    if not args.no_show:
        plt.show()


if __name__ == "__main__":
    main()
