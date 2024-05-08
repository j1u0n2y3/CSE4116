default: app.c
	@echo ">> Compiling app.c..."
	arm-none-linux-gnueabi-gcc -static -o app app.c
	@echo ">> app.c has been compiled into static binary 'app'."

push:
	@echo ">> Pushing the app to /data/local/tmp on the device..."
	adb push app /data/local/tmp
	@echo ">> The app has been pushed to /data/local/tmp on the device."

clean:
	@echo ">> Cleaning up..."
	rm -rf app
	@echo ">> Cleanup complete. The 'app' binary has been removed."

.PHONY: push clean
