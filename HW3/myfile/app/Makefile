default:
	arm-none-linux-gnueabi-gcc -static -o app app.c

push:
	@echo ">> Pushing the app to /data/local/tmp on the device..."
	make clean
	make
	adb push app /data/local/tmp
	@echo ">> The app has been pushed to /data/local/tmp on the device."

clean:
	rm -rf app

.PHONY: push clean
