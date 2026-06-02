# Date should be formatted like: do 30 apr 2026 19:55:35 CEST
COMMIT_DATE=${1}
COMMIT_MESSAGE=${2}

GIT_COMMITTER_DATE="${COMMIT_DATE}" git commit --date "${COMMIT_DATE}" -m "${COMMIT_MESSAGE}"
