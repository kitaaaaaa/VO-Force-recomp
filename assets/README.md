# assets/

`Play Virtual-On Force.bat` fills this folder from your own disc image of
**Dennou Senki Virtual-On Force** (Xbox 360, Japan, Title ID `5345084C`). Nothing in
this folder except this README is committed to git.

To fill it by hand, extract the game partition of your disc here:

```
python tools/xiso_extract.py <your image>.iso assets
```

so that it holds `default.xex` and the `media` folder. Expected `default.xex` SHA256:
`9FE5B63F76A089804F8B18D7F53F285E2743BFE5D8B43ED9A7E863DAAD5148C0`.
