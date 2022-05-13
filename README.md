# Fe

Windows 自定义热键程序

## 配置

Fe 的配置文件格式为 UTF-8 编码 [JSON](https://www.json.org/)，程序启动时会加载与程序同名，扩展名为 `.json` 的配置文件。

JSON 配置文件内的所有对象名均**不**区分大小写。

示例配置文件: [example.json](https://github.com/a1ive/fe/blob/master/example.json)

## 语法

### 按键名

格式为 [按键修饰符-]按键名，不区分大小写。

Fe 支持下列按键修饰符：`ctrl-`，`alt-`，`shift-`，`win-`。注意：涉及`WIN`键的热键被保留给操作系统使用，可能无法注册。

Fe 支持使用十六进制的按键代码 ([代码参考](https://docs.microsoft.com/en-us/windows/win32/inputdev/virtual-key-codes))，`a` - `z` 字母键，`0` - `9` 数字键，`f1` - `f11` 功能键以及下列按键名：

| 按键名     | 按键值    | 描述       |
| ---------- | --------- | ---------- |
| backspace  | VK_BACK   | 退格键     |
| tab        | VK_TAB    | TAB 键     |
| enter      | VK_RETURN | 回车键     |
| escape     | VK_ESCAPE | ESC 键     |
| space      | VK_SPACE  | 空格键     |
| pageup     | VK_PRIOR  | 向上翻页键 |
| pagedown   | VK_NEXT   | 向下翻页键 |
| end        | VK_END    | END 键     |
| home       | VK_HOME   | HOME 键    |
| leftarrow  | VK_LEFT   | 方向键左   |
| uparrow    | VK_UP     | 方向键上   |
| rightarrow | VK_RIGHT  | 方向键右   |
| downarrow  | VK_DOWN   | 方向键下   |
| select     | VK_SELECT | SELECT 键  |
| insert     | VK_INSERT | INS 键     |
| delete     | VK_DELETE | DEL 键     |

示例键名：`ctrl-alt-shift-enter`，`alt-x`，`ctrl-shift-2`，`ctrl-0x47`。

### 定义快捷键

```json
{
	"Hotkey":
	[
		{
			"Key" : "KEY1",
			...
		},
		{
			"Key" : "KEY2",
			...
		},
        ...
}
```

### 运行 EXE 程序

```json
{
	"Key" : "KEY NAME",
	"Exec" : "C:\\XXX.exe",
	"Window" : "hide"
}
```

`Exec` 项为程序路径及命令行参数，`%变量%` 会被自动扩展。注意反斜杠 `\` 应使用 `\\` 转义。示例：`cmd.exe /c a.bat`，`%windir%\\notepad.exe`。

`Window` 项可选，用于指定窗口状态，默认值为 `normal`，其他可选值为 `hide` (隐藏)，`min` (最小化)，`max` (最大化)。

### 杀死进程

```json
{
	"Key" : "KEY NAME",
	"Kill" : "XXX.exe"
}
```

`Kill` 项为要结束的进程名 (带 `.EXE`)。

### 设置分辨率

```json
{
	"Key" : "KEY NAME",
	"Monitor" : "MONITOR NAME",
	"Resolution" : "WIDTHxHEIGHT"
}
```

`Resolution` 项为分辨率，格式为 `宽x高`，例如 `1024x768`。

`Monitor` 项可选，用于指定显示设备，例如 `\\\\.\\DISPLAY1` 。

### 设置老板键 (显示/隐藏某窗口)

```json
{
	"Key" : "KEY NAME",
	"Find" : "STRING",
	"Hide" : "hide",
	"Show" : "restore"
}
```

`Find` 项为查找窗口标题中的字符串，包含该字符串的所有窗口都会被处理。

`Hide` 项可选，用于指定第一次按下该快捷键时设置的窗口状态，默认为 `hide` (隐藏)，其他可选值为 `min` (最小化)。

`Show` 项可选，用于指定第二次按下该快捷键时设置的窗口状态，默认为 `restore` (恢复)，其他可选值为 `min` (最小化)，`max` (最大化)，`min` (最小化)，`show` (显示)。

### 截图

```json
{
	"Key" : "KEY NAME",
	"Screenshot" : "current",
	"Save" : "XXX"
}
```

`Screenshot` 项指定截图范围，`all` 为全屏，`current` 为当前窗口。

`Save` 项可选，用于指定保存位置，默认值为 `clipboard` (保存到剪贴板)。若设置为 `ask`，则会弹出窗口用于选择图片保存位置。设置为其他值则会保存为 `指定值-当前时间.png`。

### 为热键设置描述文本

使用 `Note` 项来为热键设置描述文本，在系统托盘图标菜单中选择 `List`来查看注册的热键和对应描述文本。

## 许可协议

[GPLv3](https://www.gnu.org/licenses/gpl-3.0.en.html)

本软件使用了以下项目的代码：

[cJSON](https://github.com/DaveGamble/cJSON) [LICENSE](https://github.com/DaveGamble/cJSON/blob/master/LICENSE)

[LodePNG](https://github.com/lvandeve/lodepng) [LICENSE](https://github.com/lvandeve/lodepng/blob/master/LICENSE)
