# RC car control client

This is simple GLFW based program to send control data to ESP. GLFW is used to handle key events.

## Usage
Launch program with `-addr $host:$port` and focus created window, after that you'll be able to control car via `w`, `s`, `a`, `d`
like in racing games. Window will be black every time, this is expected behavior.

## Compile
Since it's not planned to keep sources in `$GOPATH` it's necessary to have glfw in your `$GOPATH`, i.e. there is no way to keep it
in `./vendor/` locally: `go get -u github.com/go-gl/glfw/v3.2/glfw`

After that you can simply run it via `go run main.go -addr $host:$port`
