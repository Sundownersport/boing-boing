#!/bin/sh
set -e

repo=Sundownersport/boing-boing
before=$(gh run list -R $repo --workflow build.yml -L 1 --json databaseId -q '.[0].databaseId')
gh workflow run build.yml -R $repo --ref main

run=$before
while [ "$run" = "$before" ]; do
    sleep 2
    run=$(gh run list -R $repo --workflow build.yml -L 1 --json databaseId -q '.[0].databaseId')
done

gh run watch $run -R $repo --exit-status
rm -f bin/*.gtr
gh run download $run -R $repo -n boing-rom -D bin
ls -l bin/*.gtr
