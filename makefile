build:
	cc -Isrc/include `pkg-config --cflags gtk+-3.0` src/*.c src/ss/*.c -o zenmonitor `pkg-config --libs gtk+-3.0` -lm -no-pie -Wall

clean:
	rm -f zenmonitor
