PIO := $(shell command -v pio 2>/dev/null || echo ~/.platformio/penv/bin/pio)

# Cached port assignments — written by set-tx / set-rx, gitignored
-include .ports
TX_PORT ?=
RX_PORT ?=

# PORT= on the command line overrides the cached value for one command only
_TX_PORT := $(or $(PORT),$(TX_PORT))
_RX_PORT := $(or $(PORT),$(RX_PORT))
_TX_UPLOAD := $(if $(_TX_PORT),--upload-port $(_TX_PORT),)
_RX_UPLOAD := $(if $(_RX_PORT),--upload-port $(_RX_PORT),)
_TX_MON    := $(if $(_TX_PORT),--port $(_TX_PORT),)
_RX_MON    := $(if $(_RX_PORT),--port $(_RX_PORT),)

# --fail-on-defect level halts the build when a defect at/above this severity is found.
# Medium is the enforced floor: no medium/high defects may be present to build.
# Override on the command line, e.g. make build-tx FAIL_ON=low
FAIL_ON ?= medium

.PHONY: help test check check-tx check-rx build-tx build-rx upload-tx upload-rx monitor-tx monitor-rx ports set-tx set-rx clean

help:
	@echo "Usage: make <target> [PORT=/dev/cu.usbserial-xxx]"
	@echo ""
	@echo "  ports         List USB serial devices and cached assignments"
	@echo "  set-tx        Save transmitter port  (make set-tx PORT=...)"
	@echo "  set-rx        Save receiver port     (make set-rx PORT=...)"
	@echo "  test          Run native unit tests"
	@echo "  check         Run static analysis on both firmwares"
	@echo "  check-tx      Run static analysis on transmitter"
	@echo "  check-rx      Run static analysis on receiver"
	@echo "  build-tx      Static-check, then compile transmitter firmware"
	@echo "  build-rx      Static-check, then compile receiver firmware"
	@echo "  upload-tx     Upload transmitter firmware"
	@echo "  upload-rx     Upload receiver firmware"
	@echo "  monitor-tx    Open serial monitor (transmitter baud)"
	@echo "  monitor-rx    Open serial monitor (receiver baud)"
	@echo "  clean         Remove build artifacts"

ports:
	@echo "Available USB serial ports:"
	@$(PIO) device list | awk '/\/dev\//{port=$$1} /USB/{print "  " port}' | sort -u
	@echo ""
	@echo "Cached assignments (saved in .ports):"
	@echo "  TX: $(or $(TX_PORT),(not set))"
	@echo "  RX: $(or $(RX_PORT),(not set))"

set-tx:
	$(if $(PORT),,$(error PORT is required, e.g. make set-tx PORT=/dev/cu.usbserial-xxx))
	@printf 'TX_PORT := $(PORT)\nRX_PORT := $(RX_PORT)\n' > .ports
	@echo "Transmitter port saved: $(PORT)"

set-rx:
	$(if $(PORT),,$(error PORT is required, e.g. make set-rx PORT=/dev/cu.usbserial-xxx))
	@printf 'TX_PORT := $(TX_PORT)\nRX_PORT := $(PORT)\n' > .ports
	@echo "Receiver port saved: $(PORT)"

test:
	$(PIO) test -e native

check: check-tx check-rx

check-tx:
	$(PIO) check -e transmitter --fail-on-defect $(FAIL_ON)

check-rx:
	$(PIO) check -e receiver --fail-on-defect $(FAIL_ON)

build-tx: check-tx
	$(PIO) run -e transmitter

build-rx: check-rx
	$(PIO) run -e receiver

upload-tx: check-tx
	$(PIO) run -e transmitter --target upload $(_TX_UPLOAD)

upload-rx: check-rx
	$(PIO) run -e receiver --target upload $(_RX_UPLOAD)

monitor-tx:
	$(PIO) device monitor -e transmitter $(_TX_MON)

monitor-rx:
	$(PIO) device monitor -e receiver $(_RX_MON)

clean:
	$(PIO) run --target clean
