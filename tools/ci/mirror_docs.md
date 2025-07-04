# 外部仓库镜像同步

本文档介绍如何配置 GitLab CI 将代码镜像推送到外部仓库。

## 功能概述

此功能允许通过 API 触发自动将整个仓库推送到另一个外部 Git 仓库。推送使用 SSH 方式进行，确保安全和可靠的代码传输。

## 前提条件

1. 生成专用的 SSH 密钥对（不使用口令保护）
   ```bash
   ssh-keygen -t ed25519 -f gitlab_mirror_key -C "GitLab Mirror Key" -N ""
   ```

2. 将公钥添加到目标仓库的部署密钥或用户 SSH 密钥中
   - 对于 GitHub：在目标仓库的 Settings -> Deploy keys
   - 对于其他 Git 托管服务：查阅相应文档

3. 对生成的私钥进行Base64编码（为了安全地存储在CI变量中）
   ```bash
   # 在macOS或Linux上
   cat gitlab_mirror_key | base64
   
   # 在Windows上使用PowerShell
   [Convert]::ToBase64String([System.IO.File]::ReadAllBytes("gitlab_mirror_key"))
   ```

4. 在 GitLab 项目设置中添加以下 CI/CD 环境变量：
   - `SSH_PRIVATE_KEY`：SSH 私钥的Base64编码内容
   - `TARGET_REPO_SSH_URL`：目标仓库的 SSH URL（例如：`git@github.com:username/repo.git`）
   - `TARGET_BRANCH_NAME`（可选）：目标仓库的分支名（默认与源分支相同）
   - `MIRROR_MODE`（可选）：镜像模式，可选 'branch'（仅当前分支）或 'all'（所有分支和标签）
   - `PUSH_TAGS`（可选）：是否推送标签，'true' 或 'false'

## 使用方式

### 自动触发

当 `merge_github_prs.py` 脚本执行后，如果代码成功推送到 GitLab，将自动触发镜像同步管道。

### 手动触发

您也可以通过 GitLab API 手动触发镜像同步：

```bash
curl --request POST \
  --header "PRIVATE-TOKEN: <your_gitlab_token>" \
  --header "Content-Type: application/json" \
  --data '{
    "ref":"main", 
    "variables":[
      {"key":"MIRROR_EXTERNAL", "value":"true"}
    ]
  }' \
  "https://<gitlab_url>/api/v4/projects/<encoded_project_path>/pipeline"
```

## 镜像模式说明

1. **单分支模式** 
   - 只推送当前分支到目标仓库
   - 可以指定目标分支名称（通过 TARGET_BRANCH_NAME）
   - 通过 PUSH_TAGS=true 可以选择性地推送标签

## 安全注意事项

1. SSH 密钥应仅用于此镜像过程，不应用于其他目的
2. 定期轮换 SSH 密钥和访问令牌
3. 在目标仓库上，仅授予必要的权限（写入权限）
4. 使用的私钥应以 Base64 编码形式存储在 CI/CD 变量中
5. CI/CD 变量应设置为"受保护"和"已屏蔽"，以提高安全性

## 故障排除

如果遇到推送失败，可以检查以下几点：

1. 确认 SSH 私钥已正确编码并添加到 CI/CD 变量
2. 验证目标仓库的 SSH URL 格式正确
3. 确保公钥已添加到目标仓库的部署密钥中
4. 检查目标仓库的部署密钥是否有写入权限
5. 查看 CI 作业日志以获取详细错误信息
