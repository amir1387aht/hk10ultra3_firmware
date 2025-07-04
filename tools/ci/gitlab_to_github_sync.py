import os
import sys
from git import Repo, GitCommandError
import gitlab
from github import Github
from github import Auth

GITLAB_BRANCH = os.environ.get('GITLAB_BRANCH', 'main')
GITHUB_TOKEN = os.environ.get('GITHUB_TOKEN')
GITHUB_REPO = os.environ.get('GITHUB_REPO')
GITHUB_BRANCH = os.environ.get('GITHUB_BRANCH', 'main')

auth = Auth.Token(GITHUB_TOKEN)
g = Github(auth=auth)

if not all([GITHUB_TOKEN, GITHUB_REPO]):
    print('缺少必要的环境变量')
    sys.exit(1)

repo = Repo(os.getcwd())
github_url = f'https://{GITHUB_TOKEN}@github.com/{GITHUB_REPO}.git'

workflow = g.get_repo(GITHUB_REPO).get_workflow('gitlab-to-github-sync.yml')
if not workflow.create_dispatch(GITHUB_BRANCH):
    print('GitHub Actions触发失败')
    sys.exit(1)

# try:
#     print(f'推送到GitHub分支 {GITHUB_BRANCH}...')
#     repo.git.push(github_url, f'{GITLAB_BRANCH}:{GITHUB_BRANCH}', force=True)
#     print('同步完成')
#     # 触发GitHub Actions
#     workflow = g.get_repo(GITHUB_REPO).get_workflow('merge_prs.yml')
#     if not workflow.create_dispatch(GITHUB_BRANCH):
#         print('GitHub Actions触发失败')
#         sys.exit(1)

# except GitCommandError as e:
#     print(f'推送失败: {e}')
#     sys.exit(1)
