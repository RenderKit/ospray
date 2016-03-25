#!/bin/bash

REF_NAME=${1}

if [[ -n "${REF_NAME}" ]]; then
  echo "Triggering CI build of branch '${REF_NAME}'"
else
  echo "No branch name specified, defaulting to 'devel'"
  REF_NAME="devel"
fi

curl -X POST \
  -F token=32b3258c799f57f1c6159585ce133b \
  -F ref=${REF_NAME} \
  https://gitlab.com/api/v3/projects/998314/trigger/builds
