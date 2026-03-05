## Dependencies

### mine-0.23
Source: T2 SDE mine package
License: GPL-2.0 (see mine-0.23/COPYING)
Modified for t2-installer:  see custom-t2-installer-mine-modern-c-port.patch

### stone
Source: T2 SDE upstream
License: GPL-2.0 (see stone/stone.desc)
Modified for t2-installer: see custom-t2-installer-stone-platform2-fallback.patch

---

## Resolving Dependencies 

### mine

I obtained the mine source by doing:

```bash
# ./t2 dl -mirror https://us.t2-dl.thenightswatch.co.uk mine
```
After which I extracted the tarball and put them in the repo.

Then I ported mine to compile with a modern compiler with the patch:
[`deps/mine-0.23/custom-t2-installer-mine-modern-c-port.patch`](deps/mine-0.23/custom-t2-installer-mine-modern-c-port.patch`)


### stone
I obtained the stone source by doing:

```bash
$ cp -r package/base/stone deps/
```
From packages/base/stone in the T2 SDE Linux upstream github repo.

Then I modified platform to fallback to 'pc'
[`deps/stone-0.23/custom-t2-installer-stone-platform2-fallback.patch`](deps/stone/t2-installer-stone-platform2-fallback.patch`)
