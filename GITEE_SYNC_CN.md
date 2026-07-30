# GitHub 同步到 Gitee

本仓库通过 `.github/workflows/sync_gitee.yml` 将 GitHub 作为主仓库，单向同步到：

```text
git@gitee.com:chengdu-changshu-robot/openarmx_firmware_ker.git
```

同步内容包括：

- GitHub 的所有分支。
- GitHub 的所有 Git 标签。
- GitHub Release 的说明和全部附件。

## GitHub Secrets

在 GitHub 仓库的 `Settings -> Secrets and variables -> Actions` 中配置：

| Secret | 用途 |
|---|---|
| `GITEE_SSH_PRIVATE_KEY` | 推送源码和标签到 Gitee 的专用 SSH 私钥 |
| `GITEE_ACCESS_TOKEN` | 调用 Gitee API 创建 Release 和上传附件 |

建议单独生成同步密钥，不要上传个人日常使用的 SSH 私钥：

```bash
ssh-keygen -t ed25519 -C "openarmx-firmware-gitee-sync" -f gitee_sync_key
```

将 `gitee_sync_key.pub` 添加到有目标仓库写入权限的 Gitee 账户 SSH 公钥中，将
`gitee_sync_key` 的完整内容保存为 GitHub Secret `GITEE_SSH_PRIVATE_KEY`。

在 Gitee 的个人设置中创建具有目标仓库和 Release 写权限的私人令牌，将其保存为
GitHub Secret `GITEE_ACCESS_TOKEN`。

## 首次同步

配置两个 Secret 后，进入 GitHub 仓库的 `Actions -> Sync to Gitee -> Run workflow`。

- `release_tag` 留空：同步所有分支和标签。
- `release_tag` 填写 `v3.2.0`：同步分支、标签，并把该 GitHub Release 的附件上传到
  Gitee Release。

之后推送分支或标签会自动同步源码；发布或编辑 GitHub Release 会自动同步 Release。
如果 Secret 尚未配置，工作流会安全跳过相应步骤并在 Summary 中给出说明。

Gitee 是只读镜像。不要在 Gitee 上维护独立分支或修改 Release，否则后续自动同步可能
覆盖同名内容。
