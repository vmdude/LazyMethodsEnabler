all: lazyMethodsEnabler

clean:
	$(RM) -f lazyMethodsEnabler

lazyMethodsEnabler: lazyMethodsEnabler.cpp
	$(CXX) -o $@ `pkg-config --cflags --libs lazyMethodsEnabler` $?
