# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## What this is

`smartmet-shapetools` — command-line tools for manipulating ESRI shapefiles, part of the SmartMet Server ecosystem (Finnish Meteorological Institute). The tools prepare and process geographic boundary data for weather map rendering.

## Build commands

```bash
make                # Build all tools (produces executables in project root)
make clean          # Remove build artifacts
make format         # Run clang-format on all source
make test           # Run tests (currently no test suite)
make rpm            # Build RPM package
make install        # Install binaries to $PREFIX/bin (default /usr)
```

Build requires SmartMet libraries (`imagine`, `newbase`, `macgyver`, `gis`) and GDAL to be installed. Build configuration comes from `smartbuildcfg` via `makefile.inc`.

## Architecture

This is a flat tools project, not a library. Each `.cpp` in `main/` compiles to a standalone executable.

**Shared library code** (`source/`, `include/`): Geometry primitives and utilities shared across tools:
- `Projection` — wraps newbase `NFmiArea` projections (stereographic, latlon, mercator, etc.) with lazy construction
- `Polygon`, `Polyline`, `Point` — geometry types with clipping operations
- `Nodes`, `Edge`, `Edges` — graph structures for triangulation/amalgamation
- `PointSelector` — distance-based point filtering
- `GradsTools` — GrADS binary format I/O

**Tools by category:**

- **Inspection**: `shapedump` (coordinates), `shape2xml` (attributes + paths), `gradsdump` (GrADS binary)
- **Rendering**: `shape2ps` (PostScript with embedded DSL for projections, contours, querydata), `shape2svg`
- **Filtering/selection**: `shapefilter` (by attribute/bbox), `shapepick` (by record), `shapefind` (by attribute query), `shapepoints` (distance-based thinning)
- **Format conversion**: `shape2grads`/`grads2shape`, `gshhs2grads`/`gshhs2shape`, `svg2shape`, `etopo2shape`, `lights2shape`
- **Amalgamation pipeline**: `shape2triangle` → external `triangle` → `amalgamate` → `triangle2shape` (combines nearby polygons via constrained Delaunay triangulation)
- **Other**: `shapeproject` (coordinate reprojection), `shapepack` (polygon simplification), `compositealpha` (image alpha compositing)

`shape2ps` is the most complex tool — it implements a mini PostScript DSL that can load shapefiles, set projections, render querydata contours, and produce full weather map output.

## Key dependencies

All tools link against the SmartMet library stack:
- `imagine` — shapefile I/O (`NFmiGeoShape`, `NFmiEsriShape`, `NFmiPath`), image rendering
- `newbase` — weather data (`NFmiQueryData`), projections (`NFmiArea`), command-line parsing (`NFmiCmdLine`, `NFmiSettings`)
- `macgyver` — general utilities
- `gis` — coordinate transformations, geometry
- GDAL, Boost (iostreams, program_options)

## CI

CircleCI builds RPMs on RHEL 8 and RHEL 10 using `fmidev/smartmet-cibase` Docker images. No automated test step in CI (build-only).
