# prosp3ct 1.1
A C++ OSINT-oriented Bing scraper that is designed for speed and efficiency.


## Getting started
Following are the instructions to build prosp3ct on your Linux system.

## Getting prerequisities
### Debian / Ubuntu
```
apt-install openssl-dev
```
### RedHat
```
yum install openssl-devel
```
### Arch
```
pacman -S openssl
```

## Compiling
```
chmod +x compile_prosp3ct.sh
./compile_prosp3ct.sh
```

## Usage
```
./prosp3ct.bin <-q string>/<-i string> [-p int] [-o string] [-s string] [-vh]
```
### Examples
Get 5 pages of results by the search parameter (query) "github"
```
./prosp3ct.bin -q "github" -p 5
```
Get 5 pages of results for the query "prosp3ct" in github.com
```
./prosp3ct.bin -q "prosp3ct" -p 5 -s "github.com"
```
Load search parameters from a file named "input.txt"
```
./prosp3ct.bin -i input.txt
```
Load search parameters from the same file and output the results to a file named "output.txt"
```
./prosp3ct.bin -i input.txt -o output.txt
```

https://www.povonsec.com
