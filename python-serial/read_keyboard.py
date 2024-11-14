#!/usr/bin/env python3
import serial
import threading
import time
import sys
import select
import termios
import tty

def serial_reader(paused_event, stop_event):
    """
    Reads from the serial port and prints the lines.
    Pauses reading when paused_event is set.
    """
    while not stop_event.is_set():
        if paused_event.is_set():
            time.sleep(0.1)
            continue
        try:
            with serial.Serial(
                port='/dev/ttyACM0',
                baudrate=115200,
                timeout=1,
                xonxoff=False,
                rtscts=False,
                dsrdtr=True
            ) as ser:
                while not paused_event.is_set() and not stop_event.is_set():
                    line = ser.readline()
                    if line:
                        print(line.decode('utf-8', errors='ignore').strip())
        except serial.SerialException:
            # Serial port not available, wait and retry
            time.sleep(0.5)
        except Exception as e:
            print(f'An error occurred: {e}')
            time.sleep(0.5)

def key_listener(paused_event, stop_event):
    """
    Listens for 'P' key presses to pause and resume the serial reader.
    """
    fd = sys.stdin.fileno()
    old_settings = termios.tcgetattr(fd)
    try:
        # Set terminal to cbreak mode
        tty.setcbreak(fd)
        while not stop_event.is_set():
            # Wait for input with a timeout
            rlist, _, _ = select.select([fd], [], [], 0.1)
            if rlist:
                ch = sys.stdin.read(1)
                if ch:
                    if ch.lower() == 'p':
                        if paused_event.is_set():
                            paused_event.clear()
                            print('Resumed')
                        else:
                            paused_event.set()
                            print('Paused')
                    elif ch.lower() == 'q':
                        stop_event.set()
                        print('Exiting...')
    except Exception as e:
        print(f'Key listener error: {e}')
    finally:
        # Restore terminal settings
        termios.tcsetattr(fd, termios.TCSADRAIN, old_settings)
        stop_event.set()

def main():
    paused_event = threading.Event()
    stop_event = threading.Event()

    serial_thread = threading.Thread(target=serial_reader, args=(paused_event, stop_event))
    key_thread = threading.Thread(target=key_listener, args=(paused_event, stop_event))

    serial_thread.start()
    key_thread.start()

    try:
        while not stop_event.is_set():
            time.sleep(0.1)
    except KeyboardInterrupt:
        print('KeyboardInterrupt received. Exiting...')
        stop_event.set()
    finally:
        paused_event.set()
        serial_thread.join()
        key_thread.join()
        # Ensure terminal settings are restored
        sys.stdin.flush()

if __name__ == '__main__':
    main()
