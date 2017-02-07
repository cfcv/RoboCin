all:
	g++ track.cpp -o track `pkg-config --cflags --libs opencv`
execute:
	./track
clean:
	rm -rf *o track
