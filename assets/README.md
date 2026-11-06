# assets/

Put your own disc image (`.iso`) of **Dennou Senki Virtual-On Force** (Xbox 360, Japan,
Title ID `5345084C`) in this folder. `force.exe` unpacks it here the first time it starts.
Nothing in this folder except this README is committed to git.

To fill it by hand, extract the game partition of your disc here:

```
python tools/xiso_extract.py <your image>.iso assets
```

so that it holds `default.xex` and the `media` folder. Expected `default.xex` SHA256:
`9FE5B63F76A089804F8B18D7F53F285E2743BFE5D8B43ED9A7E863DAAD5148C0`.

DLC package files put here keep their original file names. `force.exe` installs them into
the content folder the next time it starts.
