## Copyright 2021 Intel Corporation
## SPDX-License-Identifier: Apache-2.0

set(COMPONENT_NAME anari-ospray)

ExternalProject_Add(${COMPONENT_NAME}
  SOURCE_DIR ${COMPONENT_NAME}/src
  GIT_REPOSITORY $ENV{CI_SERVER_PROTOCOL}://gitlab-ci-token:$ENV{CI_JOB_TOKEN}@$ENV{CI_SERVER_HOST}/$ENV{CI_PROJECT_NAMESPACE}/${COMPONENT_NAME}.git
  GIT_TAG origin/devel
)
ExternalProject_Add_StepTargets(${COMPONENT_NAME} NO_DEPENDS download)
