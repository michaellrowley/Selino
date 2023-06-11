# Selino
<p align="center">
<img src="./logo.png" width=150>
</p>

Selino is a cross-platform, **scriptable** proxy with support for ``CONNECT``-type connections on SOCKS4(/a) and SOCKS5(/h).

<span align="center">

[![Codacy Badge](https://api.codacy.com/project/badge/Grade/e6477bac125f4614940c1821655fd228)](https://app.codacy.com/gh/michaellrowley/Selino?utm_source=github.com&utm_medium=referral&utm_content=michaellrowley/Selino&utm_campaign=Badge_Grade)
[![Lua - Documentation](https://img.shields.io/badge/Lua_API-Documentation-blue?style=for-the-badge)](https://proxy-application.gitbook.io/ "Go to project documentation")

</span>

## Usage
Running this application is as simply as launching its binary followed by some (*mostly* optional) standard command-line arguments;
- ``-s`` / ``--script``: A script to receive app-specific callbacks throughout the application's execution.
- ``-p`` / ``--port``: The port that the application should listen on for incoming connections.
- ``-t`` / ``--timeout``: The application's inactivity timeout during two-way 'transparent' forwarding. This can be specified as ``60,000`` (defaults to milliseconds), or with a suffix such as ``s``/``ms``/``m`` or ``seconds``/``milliseconds``/``minutes``.
> selino -s log_data.lua -p 8080 -p 1080 -p  -t 30ms

Lua scripts play a major role in this project's feature-set, allowing users to expand the proxy's capability or specialize features towards a specific goal.

## Example
> $ selino -s [testing/logging.lua](./testing/logging.lua) -p 8080 -p 1080 -p -t 30ms

Separate terminal:
> $ curl -x socks5://localhost:8080 http://info.cern.ch

<details>
<summary>
Output from original terminal
</summary>

```bash
Initialization function called in logging.lua
Received a connection on local '127.0.0.1:8080' from '127.0.0.1:62173'
Packet received:
	Protocol: 'SOCKS5'
	Packet Index: '0'
	Local Endpoint: '127.0.0.1:8080'
	Remote Endpoint: '127.0.0.1'
	Remote Port: '62173'
	Raw Packet: '\x05\x02\x00\x01'
Packet received:
	Protocol: 'SOCKS5'
	Packet Index: '1'
	Local Endpoint: '127.0.0.1:8080'
	Remote Endpoint: '127.0.0.1'
	Remote Port: '62173'
	Raw Packet: '\x05\x01\x00\x01\xbc\xb8\x15\x6c\x00\x50\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00'
Received a SOCKS5 request:
	Incoming Port: 'nil'
	Incoming Endpoint: '127.0.0.1:62173'
	Command: 'CONNECT'
	Destination: '188.184.21.108:80'
Forwarding '76' bytes ('127.0.0.1:62173' ==> '188.184.21.108:80':
	Raw Data: '\x47\x45\x54\x20\x2f\x20 ... \x2a\x0d\x0a\x0d\x0a')
Forwarding '878' bytes ('188.184.21.108:80' ==> '127.0.0.1:62173':
	Raw Data: '\x48\x54\x54\x50\x2f\x31 ... \x74\x6d\x6c\x3e\x0a')
Tunnel ('127.0.0.1:62173' <==> '188.184.21.108:80') has been closed
Terminated a connection on local '127.0.0.1:8080' from '127.0.0.1:62173'
```
</details>

## Installation
Compiling this project requires inclusion of some external libraries:
- [Sol2](https://github.com/ThePhD/sol2)(v3) for implementation of Lua scripting
- [Lua](https://www.lua.org/download.html) (tested using ``5.4``) for Sol to use as a backend for the language
- [Boost](https://www.boost.org/) for cross-platform networking

I looked into creating a makefile for this project, however linking to boost made it too complicated to work between environments without changing a system's configuration.