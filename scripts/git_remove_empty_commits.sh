#!/usr/bin/env bash
git filter-branch --commit-filter 'git_commit_non_empty_tree "$@"' HEAD
git reflog expire --all &&  git gc --aggressive --prune 
