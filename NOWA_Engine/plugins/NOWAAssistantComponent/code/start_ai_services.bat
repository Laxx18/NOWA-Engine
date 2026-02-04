@echo off
setlocal

echo [AI] Starting local services...

REM Change this path to where your compose file lives:
set COMPOSE_DIR=%~dp0docker

if not exist "%COMPOSE_DIR%" (
  echo [AI] ERROR: compose folder not found: %COMPOSE_DIR%
  echo [AI] Edit Ai\start_ai_services.bat and set COMPOSE_DIR correctly.
  exit /b 1
)

pushd "%COMPOSE_DIR%"

echo [AI] docker compose up -d
docker compose up -d

popd

echo [AI] Done. Qdrant should be reachable at http://localhost:6333
echo [AI] Ollama should be reachable at http://localhost:11434

endlocal
