The Windows client may choose which keypress is simulated when notified by the other host.

Server is assumed to run X.Org.

Example usage:

`bkbslave.exe host port vkcode`

`bkbslave.exe 127.0.0.1 27015 0x5a`

See https://docs.microsoft.com/en-us/windows/win32/inputdev/virtual-key-codes

Built with gcc 11.2.0 and msvc 14.31
