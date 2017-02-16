all: sample2D

sample2D: game.cpp glad.c
	g++ -o sample2D game.cpp glad.c -lGL -lglfw -ldl -lao -lmpg123

clean:
	rm sample2D
