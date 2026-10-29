# MINbox

## Memory limit

The `--memory` option accepts a positive whole number of bytes or a size with a unit:

```sh
minbox run --rootfs ./rootfs --memory 64M -- /bin/sh
```

`K`, `M`, `G`, and `T` (also `KiB`, `MiB`, `GiB`, and `TiB`) use powers of 1024. The explicit `KB`, `MB`, `GB`, and `TB` suffixes use powers of 1000. Units are case-insensitive, so `64M` and `64m` both mean 64 MiB.
