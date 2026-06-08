### Ideapad Keyboard Backlight
Control keyboard backlight on your own! Now also supports switching ITS(Intelligent Thermal System) modes!

Should work on Lenovo Ideapad & Xiaoxin models. Also works on Thinkbook models as reported by several users.

Make sure Lenovo energy management driver is installed.

Run with:
```
$ <app.exe> --kbd [0|1|2|3] --its [0|1|2|3]
```

`--kbd` controls keyboard backlight, while `--its` controls ITS from Lenovo. The ITS code is largely borrowed from [LenovoLegionToolkit](https://github.com/LenovoLegionToolkit-Team/LenovoLegionToolkit).

Add it to task scheduler for autostart, or simply run `setup.bat` as administrator.
