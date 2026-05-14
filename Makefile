SHELL := /usr/bin/env bash

ROOT_DIR := $(CURDIR)
SCRIPT_DIR := $(ROOT_DIR)/scripts
DATA_DIR := $(ROOT_DIR)/test_data
OUTPUT_DIR := $(ROOT_DIR)/output

ANALYZER := $(SCRIPT_DIR)/analyze.sh
REPORTER := $(SCRIPT_DIR)/generate_report.sh
SETUP := $(SCRIPT_DIR)/setup_env.sh

LOG_FILES := $(wildcard $(DATA_DIR)/*.log)

.PHONY: all test report clean help setup

all: ## Run analyzer on all test log files
	@mkdir -p $(OUTPUT_DIR)
	@for f in $(LOG_FILES); do \
		out="$(OUTPUT_DIR)/$$(basename "$$f").report"; \
		$(ANALYZER) "$$f" --output "$$out"; \
	done

test: ## Run basic checks against sample logs
	@mkdir -p $(OUTPUT_DIR)
	@set -e; \
	$(ANALYZER) "$(DATA_DIR)/sample_pass.log" --output "$(OUTPUT_DIR)/sample_pass.log.report"; \
	if $(ANALYZER) "$(DATA_DIR)/sample_fail.log" --output "$(OUTPUT_DIR)/sample_fail.log.report"; then \
		echo "Expected sample_fail.log to fail but it passed"; \
		exit 1; \
	else \
		echo "sample_fail.log correctly reported failures"; \
	fi; \
	$(ANALYZER) "$(DATA_DIR)/sample_sim.log" --output "$(OUTPUT_DIR)/sample_sim.log.report" >/dev/null

report: ## Generate an HTML summary report in output/
	@$(REPORTER) "$(OUTPUT_DIR)/report.html"

clean: ## Remove generated output files
	@rm -rf $(OUTPUT_DIR)

setup: ## Check required CLI tools
	@$(SETUP)

help: ## Show this help message
	@awk 'BEGIN {FS=":.*##"; print "Available targets:"} /^[a-zA-Z_-]+:.*##/ {printf "  %-12s %s\n", $$1, $$2}' $(MAKEFILE_LIST)
