# http-server

A simple HTTP/1.0 server (not yet compliant to HTTP spec)
At the moment the code is a mess, will be updated later

HTTP/1.0 spec : https://datatracker.ietf.org/doc/html/rfc1945

## How to build

Run cmake to generate makefiles
```bash
cmake .
```

Run make to compile the program
```bash
make .
```

To run the service
```bash
./bin/server <PORT>
```

NB: `<PORT>` must be an available port , if not provided the program fallsback to 3000

After that if you visit `http://localhost:<PORT>` with your browser you should receive a Hey message :)
