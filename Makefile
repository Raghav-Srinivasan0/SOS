all: clean build
build: Shell-Scripts/build.sh Shell-Scripts/verify-bin.sh Shell-Scripts/make-iso.sh
	bash Shell-Scripts/build.sh
run: Shell-Scripts/run.sh
	bash Shell-Scripts/run.sh
clean:
	rm -rf Objects/*
	rm -rf myos.iso
