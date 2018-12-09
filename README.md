# helics-ns3

[![Build Status](https://dev.azure.com/HELICS-test/helics-ns3/_apis/build/status/GMLC-TDC.helics-ns3?branchName=master)](https://dev.azure.com/HELICS-test/helics-ns3/_build/latest?definitionId=1?branchName=master)

helics-ns3 is an [ns-3](https://www.nsnam.org/) module for coupling network simulations with other simulators using [HELICS](https://www.helics.org/).

## Prerequisites

Install a recent 1.x version of [HELICS](https://github.com/GMLC-TDC/HELICS-src).

Get a copy of the latest development version of ns-3 from their mercurial or git repository. The 3.28 and 3.29 stable releases both have a bug with finding Boost; any version after Dec 6, 2018 should work, or prior to Mar 2018.

Mercurial:
```bash
hg clone http://code.nsnam.org/ns-3-dev/
```
Git:
```bash
git clone https://github.com/nsnam/ns-3-dev-git
```

## Installation

Clone a copy of this repository into a folder named `helics` in the ns-3 `contrib` directory. The module directory name *must* be helics, otherwise the ns-3 build system will be confused.

```bash
cd ns-3-dev/
git clone https://github.com/GMLC-TDC/helics-ns3 contrib/helics
```

Run `./waf configure` with the `--disable-werror` option, and set the `--with-helics` option to the path of your HELICS installation. To enable examples or tests use `--enable-examples` or `--enable-tests`, respectively. If the system copy of Boost was not used to build HELICS (or there are several versions of Boost installed), then use the `--boost-includes` and `--boost-libs` options to tell waf where the copy of Boost to use is located. If ZMQ is not found, `--with-zmq` can be used to specify where it is installed. Paths should be absolute.

After configuration is done, run `./waf build` to compile ns-3 with the HELICS module.

```bash
./waf configure --with-helics=/usr/local --disable-werror --enable-examples --enable-tests
./waf build
```

## Contributing
Pull requests are welcome. For major changes, please open an issue first to discuss what you would like to change.
