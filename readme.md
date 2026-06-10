### Ideapad Keyboard Backlight
Control keyboard backlight on your own! Now also supports switching charging modes and ITS(Intelligent Thermal System) modes!

Should work on Lenovo Ideapad & Xiaoxin models. Also works on Thinkbook models as reported by several users.

Make sure Lenovo energy management driver is installed.

Run with:
```
$ <app.exe> --backlight [0|1|2|3] --charge [0|1|2|3] --its [0|1|2|3]
```

- `--backlight` controls keyboard backlight level: `off`, `lv1`, `lv2`, `auto`.
- `--charge` controls charging mode: `normal`, `quick`, `storage`, `night`.
- `--its` controls ITS mode: `auto`, `cool`, `performance`, `geek`. The ITS code is largely borrowed from [LenovoLegionToolkit](https://github.com/LenovoLegionToolkit-Team/LenovoLegionToolkit).

Add it to task scheduler for autostart, or simply run `setup.bat` as administrator.
