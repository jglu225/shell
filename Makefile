OBJECTS = msh.o 

CFLAGS = -Wall

proj4: $(OBJECTS)
	g++ $^ -o $@

%.o: %.cpp $(HEADERS)
	g++ -c $< -o $@

clean:
	rm -i *.o proj4



