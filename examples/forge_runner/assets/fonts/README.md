# FORGE fonts

英文正文/标题使用 Barlow Medium 和 Rajdhani SemiBold，许可分别为
`Barlow-OFL.txt` 和 `Rajdhani-OFL.txt`。

中文使用 Noto Sans SC（思源黑体系列）的 500 / 600 字重，修改后的子集字体
命名为 **FORGE SC Medium / SemiBold**，保留上游版权及 SIL OFL 1.1 许可。
正文采用 Medium，按钮/标题采用 SemiBold。Noto 原始许可见
`NotoSansSC-OFL.txt`。子集覆盖中文目录和 ASCII，正常构建无需下载字体。

上游：[google/fonts Noto Sans SC](https://github.com/google/fonts/tree/main/ofl/notosanssc)
的 `NotoSansSC[wght].ttf`。本次源文件 SHA-256：
`a3041811a78c361b1de50f953c805e0244951c21c5bd412f7232ef0d899af0da`。
该文件含 Google 字体 metadata 指向的 noto-cjk 来源记录；上游 URL 可能更新，
重现时应核对 hash。准备脚本使用 fonttools 4.65.0。

```powershell
python examples/forge_runner/tools/prepare-chinese-fonts.py <NotoSansSC-variable.ttf>
```

增加中文译文时，更新 `assets/locales/zh-CN.tsv` 后重跑字体准备脚本，否则
新字符可能没有字形。脚本检查上游字符覆盖，固定字重、裁剪和重命名字体，
保留版权/许可记录。运行时只对目录中的字符建图集，按实际字号两倍光栅化并
双线性采样；中文字面比例与最小字号独立处理，英文保留原字体。
