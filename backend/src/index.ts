import express from 'express';
import cors from 'cors';
import { spawn } from 'child_process';
import path from 'path';

const app = express();
const PORT = process.env.PORT || 3001;

// Path to the interpreter binary
const INTERPRETER_PATH = path.resolve(__dirname, '../../interpreter/build/minilang.exe');

// Limits
const MAX_SOURCE_SIZE = 100 * 1024; // 100KB
const MAX_OUTPUT_SIZE = 1 * 1024 * 1024; // 1MB
const EXECUTION_TIMEOUT = 3000; // 3 seconds

app.use(cors());
app.use(express.json({ limit: '200kb' }));

// Health check
app.get('/api/health', (_req, res) => {
  res.json({ status: 'ok', interpreter: INTERPRETER_PATH });
});

// Execute MiniLang source code
app.post('/api/execute', (req, res) => {
  const { source } = req.body;

  // Validate request
  if (typeof source !== 'string') {
    res.status(400).json({
      success: false,
      output: [],
      tokens: [],
      ast: null,
      symbols: [],
      errors: [{ phase: 'api', type: 'bad_request', message: 'Missing or invalid "source" field', line: 0, col: 0 }],
      phases: { lexer: 'skipped', parser: 'skipped', semantic: 'skipped', runtime: 'skipped' },
      executionTimeMs: 0
    });
    return;
  }

  // Check source size
  if (Buffer.byteLength(source, 'utf8') > MAX_SOURCE_SIZE) {
    res.status(413).json({
      success: false,
      output: [],
      tokens: [],
      ast: null,
      symbols: [],
      errors: [{ phase: 'api', type: 'source_too_large', message: `Source code exceeds ${MAX_SOURCE_SIZE / 1024}KB limit`, line: 0, col: 0 }],
      phases: { lexer: 'skipped', parser: 'skipped', semantic: 'skipped', runtime: 'skipped' },
      executionTimeMs: 0
    });
    return;
  }

  // Spawn interpreter process (no shell, fixed args)
  const child = spawn(INTERPRETER_PATH, [], {
    stdio: ['pipe', 'pipe', 'pipe'],
    shell: false,
    timeout: EXECUTION_TIMEOUT
  });

  let stdout = '';
  let stderr = '';
  let killed = false;

  // Set timeout
  const timer = setTimeout(() => {
    killed = true;
    child.kill('SIGKILL');
  }, EXECUTION_TIMEOUT);

  child.stdout.on('data', (data: Buffer) => {
    stdout += data.toString();
    if (stdout.length > MAX_OUTPUT_SIZE) {
      killed = true;
      child.kill('SIGKILL');
    }
  });

  child.stderr.on('data', (data: Buffer) => {
    stderr += data.toString();
  });

  child.on('close', (code) => {
    clearTimeout(timer);

    if (killed) {
      res.status(408).json({
        success: false,
        output: [],
        tokens: [],
        ast: null,
        symbols: [],
        errors: [{ phase: 'api', type: 'timeout', message: 'Execution timed out (3s limit)', line: 0, col: 0 }],
        phases: { lexer: 'skipped', parser: 'skipped', semantic: 'skipped', runtime: 'skipped' },
        executionTimeMs: EXECUTION_TIMEOUT
      });
      return;
    }

    try {
      const result = JSON.parse(stdout);
      res.json(result);
    } catch (e) {
      res.status(500).json({
        success: false,
        output: [],
        tokens: [],
        ast: null,
        symbols: [],
        errors: [{ phase: 'api', type: 'parse_error', message: `Failed to parse interpreter output: ${stderr || 'Unknown error'}`, line: 0, col: 0 }],
        phases: { lexer: 'skipped', parser: 'skipped', semantic: 'skipped', runtime: 'skipped' },
        executionTimeMs: 0
      });
    }
  });

  child.on('error', (err) => {
    clearTimeout(timer);
    res.status(500).json({
      success: false,
      output: [],
      tokens: [],
      ast: null,
      symbols: [],
      errors: [{ phase: 'api', type: 'spawn_error', message: `Failed to start interpreter: ${err.message}`, line: 0, col: 0 }],
      phases: { lexer: 'skipped', parser: 'skipped', semantic: 'skipped', runtime: 'skipped' },
      executionTimeMs: 0
    });
  });

  // Write source to stdin and close
  child.stdin.write(source);
  child.stdin.end();
});

app.listen(PORT, () => {
  console.log(`MiniLang API server running on http://localhost:${PORT}`);
  console.log(`Interpreter: ${INTERPRETER_PATH}`);
});
