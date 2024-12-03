# Arduino CLI Makefile

# Variables
BOARD_FQBN = arduino:avr:leonardo      # Fully Qualified Board Name for Arduino Leonardo
SKETCH = a500kybhid.ino  			   # Path to sketch file
BUILD_DIR = build                      # Directory for build artifacts
LIBRARIES = Keyboard			       # List of required libraries
CORE = arduino:avr                     # Required core

# Commands
ARDUINO_CLI = arduino-cli
VERIFY_CMD = $(ARDUINO_CLI) compile --fqbn $(BOARD_FQBN) --warnings more --verify --build-path $(BUILD_DIR) $(SKETCH)
UPLOAD_CMD = $(ARDUINO_CLI) upload --fqbn $(BOARD_FQBN) --verbose --input-dir $(BUILD_DIR) --port $(shell $(ARDUINO_CLI) board list | grep -m 1 tty | awk '{print $$1}')
INSTALL_LIBRARIES_CMD = $(ARDUINO_CLI) lib install $(LIBRARIES)
UPDATE_CORES_LIBRARIES_CMD = $(ARDUINO_CLI) core update-index && $(ARDUINO_CLI) lib update-index && $(ARDUINO_CLI) core upgrade && $(ARDUINO_CLI) lib upgrade
INSTALL_CORE_CMD = $(ARDUINO_CLI) core update-index && $(ARDUINO_CLI) core install $(CORE)
INSTALL_ARDUINO_CLI_CMD = curl -fsSL https://raw.githubusercontent.com/arduino/arduino-cli/master/install.sh | sh

# Targets
all: verify

verify:
	$(VERIFY_CMD)

upload: verify
	$(UPLOAD_CMD)

clean:
	rm -rf $(BUILD_DIR)

install-arduino-cli:
	$(INSTALL_ARDUINO_CLI_CMD)

install-core:
	$(INSTALL_CORE_CMD)

install-libraries:
	$(INSTALL_LIBRARIES_CMD)

update-cores-libraries:
	$(UPDATE_CORES_LIBRARIES_CMD)

# Help
help:
	@echo "Usage:"
	@echo "  make verify                - Compile the sketch with warnings"
	@echo "  make upload                - Compile and upload the sketch with auto-detected port"
	@echo "  make clean                 - Remove build artifacts"
	@echo "  make install-arduino-cli   - Install Arduino CLI"
	@echo "  make install-core          - Install required core"
	@echo "  make install-libraries     - Install required libraries"
	@echo "  make update-cores-libraries - Update all cores and libraries"