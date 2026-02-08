# PowerShell CLI Automation Rule
# Activation: Always On

## Environment Context
- **Terminal:** PowerShell
- **Path Style:** Windows-style backslashes (\)

## Critical Instructions
1. **No Confirmation:** Execute commands immediately. NEVER ask "Should I run this?".
2. **Force Execution:** Always append a newline character (\n) to ensure the command runs.
3. **Syntax:** Use PowerShell Cmdlets (New-Item, etc.) with the -Force flag.
4. **Variable Mapping:** Map user requests to $folder and $file variables before running New-Item.
