# Build from source

## Frontend

You must have nodejs installed. The frontend must be built before the backend.

```sh
cd frontend
npm ci
npm run build

```

## Backend

Install the packages needed for the compilation

```sh
apt-get install libasio-dev libcurl4-gnutls-dev libjsoncpp-dev libprocps-dev colorized-logs git build-essential cmake debhelper pkg-config
```

Download crow :

```sh
mkdir backend/lib
cd backend/lib
git clone https://github.com/CrowCpp/Crow.git
```


* Note for distributions Debian 9 Stretch and Devuan 2 Ascii

  You need to install the libasio-dev_1.12.2-1_all.deb package corresponding to the [Debian 10 Buster](http://ftp.debian.org/debian/pool/main/a/asio/libasio-dev_1.12.2-1_all.deb) or [Devuan 3 Beowulf](http://deb.devuan.org/merged/pool/DEBIAN/main/a/asio/libasio-dev_1.12.2-1_all.deb) distribution

* Note for distributions Debian 10 Buster and Devuan 3 Beowulf

  There is a bug with the default compiler : ["GCC 8.1 - 8.3 has a bug that prevents Crow from compiling with C++11. Please update GCC to > 8.3 or use C++ > 11."](https://github.com/CrowCpp/Crow/blob/87cf68f0eb67f210d2295e43a999a50b2d0fc65b/include/crow/settings.h#L65)
  You must replace `CXX_STANDARD 11` with `CXX_STANDARD 17` in `backend/CMakeLists.txt`

* Note for distributions Debian 11 Bullseye and Devuan 4 Chimaera

  Devuan 4 Chimaera is the nominal development distribution.

## Debian packaging

```sh
cd backend
dpkg-buildpackage -uc -us -b -tc
```

## Manual compilation

```sh
cd backend
mkdir build
cd build
cmake ..
make
```
