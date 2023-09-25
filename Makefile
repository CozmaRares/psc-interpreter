all: run

run:
	cargo run $(ARGS)

trace:
	RUST_BACKTRACE=1 $(MAKE) run $(ARGS)

check:
	cargo clippy -- -Dwarnings

test:
	cargo test

.PHONY: all run trace check test
