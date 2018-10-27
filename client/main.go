package main

import (
	"fmt"
	"net"
	"runtime"
	"time"

	"github.com/go-gl/glfw/v3.2/glfw"
)

func init() {
	runtime.LockOSThread()
}

func socketRoutine(inC chan []byte) {
	serverAddr, err := net.ResolveUDPAddr("udp", "192.168.31.66:359") // It's temporary hardcoded
	if err != nil {
		panic(err)
	}

	conn, err := net.DialUDP("udp", nil, serverAddr)
	if err != nil {
		panic(err)
	}
	defer conn.Close()

	for {
		buf, ok := <-inC
		if !ok {
			return
		}

		conn.Write(buf)
	}
}

func sign(a int) int {
	if a >= 0 {
		return 1
	} else {
		return -1
	}
}

type keyAction struct {
	key    glfw.Key
	action glfw.Action
}

func inputRoutine(tickC <-chan time.Time, keyActionC chan keyAction, controlC chan []byte) {
	const wheelAcc = 16
	const engineAcc = 16

	maxValue := 255

	wheelValue := int(0)
	engineValue := int(0)

	pressed := struct {
		w bool
		s bool
		a bool
		d bool
	}{false, false, false, false}

	for {
		select {
		case <-tickC:
			wheelDirection := int(0)
			engineDirection := int(0)

			if pressed.w && !pressed.s {
				engineDirection = 1
			} else if pressed.s && !pressed.w {
				engineDirection = -1
			}

			if pressed.a && !pressed.d {
				wheelDirection = -1
			} else if pressed.d && !pressed.a {
				wheelDirection = 1
			}

			switch engineDirection {
			case 1:
				engineValue += engineAcc
				if engineValue > maxValue {
					engineValue = maxValue
				}
			case -1:
				engineValue -= engineAcc
				if engineValue < -maxValue {
					engineValue = -maxValue
				}
			case 0:
				s := sign(engineValue)
				newEngineValue := engineValue - s*engineAcc/8
				if s > 0 && newEngineValue < 0 {
					engineValue = 0
				} else if s < 0 && newEngineValue > 0 {
					engineValue = 0
				} else {
					engineValue = newEngineValue
				}
			default:
			}

			switch wheelDirection {
			case 1:
				wheelValue += wheelAcc
				if wheelValue > maxValue {
					wheelValue = maxValue
				}
			case -1:
				wheelValue -= wheelAcc
				if wheelValue < -maxValue {
					wheelValue = -maxValue
				}
			case 0:
				s := sign(wheelValue)
				wheelValue -= s * wheelAcc / 2
				if s > 0 && wheelValue < 0 {
					wheelValue = 0
				} else if s < 0 && wheelValue > 0 {
					wheelValue = 0
				}
			default:
			}

			left := byte(0)
			right := byte(0)
			forward := byte(0)
			backward := byte(0)

			if wheelValue < 0 {
				left = byte(-wheelValue)
			} else {
				right = byte(wheelValue)
			}

			if engineValue < 0 {
				backward = byte(-engineValue)
			} else {
				forward = byte(engineValue)
			}

			select {
			case controlC <- []byte{forward, backward, left, right}:
			default:
			}
			fmt.Println([]byte{forward, backward, left, right})
		case e := <-keyActionC:
			isPressed := false
			if e.action == glfw.Press || e.action == glfw.Repeat {
				isPressed = true
			}
			switch e.key {
			case glfw.KeyW:
				pressed.w = isPressed
			case glfw.KeyS:
				pressed.s = isPressed
			case glfw.KeyA:
				pressed.a = isPressed
			case glfw.KeyD:
				pressed.d = isPressed
			default:
			}
		default:
		}
	}
}

func getKeyHandler(keyActionC chan keyAction) glfw.KeyCallback {
	return func(w *glfw.Window, key glfw.Key, _ int, action glfw.Action, _ glfw.ModifierKey) {
		switch key {
		case glfw.KeyW:
			keyActionC <- keyAction{key: glfw.KeyW, action: action}
		case glfw.KeyS:
			keyActionC <- keyAction{key: glfw.KeyS, action: action}
		case glfw.KeyA:
			keyActionC <- keyAction{key: glfw.KeyA, action: action}
		case glfw.KeyD:
			keyActionC <- keyAction{key: glfw.KeyD, action: action}
		default:
		}
	}
}

func main() {
	err := glfw.Init()
	if err != nil {
		panic(err)
	}
	defer glfw.Terminate()

	window, err := glfw.CreateWindow(640, 480, "RC car ctrl", nil, nil)
	if err != nil {
		panic(err)
	}

	tickTime := 50 * time.Millisecond

	tickC := time.NewTicker(tickTime).C
	actionC := make(chan keyAction)
	inC := make(chan []byte)

	go socketRoutine(inC)
	go inputRoutine(tickC, actionC, inC)

	window.MakeContextCurrent()

	for !window.ShouldClose() {
		window.SetKeyCallback(getKeyHandler(actionC))
		window.SwapBuffers()
		glfw.PollEvents()
	}
}
