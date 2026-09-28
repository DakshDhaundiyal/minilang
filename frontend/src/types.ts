export interface Token {
  index: number;
  type: string;
  lexeme: string;
  line: number;
  col: number;
}

export interface ASTNode {
  type: string;
  line: number;
  col: number;
  [key: string]: unknown;
}

export interface SymbolEntry {
  name: string;
  type: string;
  value: string;
  scope: string;
}

export interface ErrorEntry {
  phase: string;
  type: string;
  message: string;
  line: number;
  col: number;
}

export interface Phases {
  lexer: 'ok' | 'error' | 'skipped';
  parser: 'ok' | 'error' | 'skipped';
  semantic: 'ok' | 'error' | 'skipped';
  runtime: 'ok' | 'error' | 'skipped';
}

export interface ExecutionResult {
  success: boolean;
  output: string[];
  tokens: Token[];
  ast: ASTNode | null;
  symbols: SymbolEntry[];
  errors: ErrorEntry[];
  phases: Phases;
  executionTimeMs: number;
}
