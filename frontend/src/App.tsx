import { useState, useCallback, useRef, useEffect } from 'react';
import {
  Play, RotateCcw, ChevronDown, Terminal, Braces, Table,
  AlertTriangle, BookOpen, Loader2, WifiOff, Zap
} from 'lucide-react';
import { EditorView, keymap, lineNumbers, highlightActiveLine, highlightActiveLineGutter, Decoration, DecorationSet } from '@codemirror/view';
import { EditorState, StateField, StateEffect } from '@codemirror/state';
import { bracketMatching } from '@codemirror/language';
import { defaultKeymap, indentWithTab, history, historyKeymap } from '@codemirror/commands';
import { miniLangLanguage } from './minilang-lang';
import { executeCode, healthCheck } from './api';
import { ExecutionResult, ErrorEntry } from './types';
import { EXAMPLES } from './examples';
import './index.css';

// --- Error line decoration ---
const setErrorLines = StateEffect.define<number[]>();

const errorLineField = StateField.define<DecorationSet>({
  create() {
    return Decoration.none;
  },
  update(decorations, tr) {
    for (const e of tr.effects) {
      if (e.is(setErrorLines)) {
        const decos: { from: number; to: number; deco: Decoration }[] = [];
        for (const line of e.value) {
          if (line >= 1 && line <= tr.state.doc.lines) {
            const lineObj = tr.state.doc.line(line);
            decos.push({
              from: lineObj.from,
              to: lineObj.from,
              deco: Decoration.line({ class: 'cm-error-line' }),
            });
          }
        }
        return Decoration.set(decos.map(d => d.deco.range(d.from)));
      }
    }
    return decorations;
  },
  provide: (f) => EditorView.decorations.from(f),
});

// --- Tabs ---
type TabId = 'tokens' | 'ast' | 'symbols' | 'errors' | 'grammar';

const TABS: { id: TabId; label: string; icon: React.ReactNode }[] = [
  { id: 'tokens', label: 'Tokens', icon: <Table size={14} /> },
  { id: 'ast', label: 'AST', icon: <Braces size={14} /> },
  { id: 'symbols', label: 'Symbols', icon: <Zap size={14} /> },
  { id: 'errors', label: 'Errors', icon: <AlertTriangle size={14} /> },
  { id: 'grammar', label: 'Grammar', icon: <BookOpen size={14} /> },
];

function App() {
  const [code, setCode] = useState(EXAMPLES[0].code);
  const [result, setResult] = useState<ExecutionResult | null>(null);
  const [loading, setLoading] = useState(false);
  const [activeTab, setActiveTab] = useState<TabId>('tokens');
  const [backendOnline, setBackendOnline] = useState(true);
  const [showExamples, setShowExamples] = useState(false);
  const editorRef = useRef<HTMLDivElement>(null);
  const viewRef = useRef<EditorView | null>(null);
  const examplesRef = useRef<HTMLDivElement>(null);

  // Run code
  const runCode = useCallback(async () => {
    setLoading(true);
    try {
      const res = await executeCode(code);
      setResult(res);
      setBackendOnline(true);

      // Highlight error lines
      if (viewRef.current && res.errors.length > 0) {
        const errorLines = res.errors.filter(e => e.line > 0).map(e => e.line);
        viewRef.current.dispatch({
          effects: setErrorLines.of(errorLines),
        });
      } else if (viewRef.current) {
        viewRef.current.dispatch({
          effects: setErrorLines.of([]),
        });
      }

      // Switch to errors tab if there are errors
      if (res.errors.length > 0) {
        setActiveTab('errors');
      }
    } catch {
      setBackendOnline(false);
      setResult(null);
    } finally {
      setLoading(false);
    }
  }, [code]);

  // Init CodeMirror
  useEffect(() => {
    if (!editorRef.current || viewRef.current) return;

    const runKeymap = keymap.of([
      {
        key: 'Ctrl-Enter',
        run: () => {
          runCode();
          return true;
        },
      },
      {
        key: 'Mod-Enter',
        run: () => {
          runCode();
          return true;
        },
      },
    ]);

    const state = EditorState.create({
      doc: code,
      extensions: [
        lineNumbers(),
        highlightActiveLine(),
        highlightActiveLineGutter(),
        bracketMatching(),
        history(),
        miniLangLanguage,
        errorLineField,
        keymap.of([...defaultKeymap, ...historyKeymap, indentWithTab]),
        runKeymap,
        EditorView.updateListener.of((update) => {
          if (update.docChanged) {
            setCode(update.state.doc.toString());
          }
        }),
        EditorView.theme({
          '&': { height: '100%' },
          '.cm-scroller': { overflow: 'auto' },
        }),
      ],
    });

    viewRef.current = new EditorView({
      state,
      parent: editorRef.current,
    });

    // Check backend health
    healthCheck().then(setBackendOnline);

    return () => {
      viewRef.current?.destroy();
      viewRef.current = null;
    };
    // eslint-disable-next-line react-hooks/exhaustive-deps
  }, []);

  // Update editor content when selecting example
  const selectExample = useCallback(
    (idx: number) => {
      const example = EXAMPLES[idx];
      setCode(example.code);
      if (viewRef.current) {
        viewRef.current.dispatch({
          changes: { from: 0, to: viewRef.current.state.doc.length, insert: example.code },
          effects: setErrorLines.of([]),
        });
      }
      setResult(null);
      setShowExamples(false);
    },
    []
  );

  // Reset
  const reset = useCallback(() => {
    setCode('');
    setResult(null);
    if (viewRef.current) {
      viewRef.current.dispatch({
        changes: { from: 0, to: viewRef.current.state.doc.length, insert: '' },
        effects: setErrorLines.of([]),
      });
    }
  }, []);

  // Jump to line in editor
  const jumpToLine = useCallback((line: number) => {
    if (!viewRef.current || line < 1) return;
    const doc = viewRef.current.state.doc;
    if (line > doc.lines) return;
    const lineObj = doc.line(line);
    viewRef.current.dispatch({
      selection: { anchor: lineObj.from },
      scrollIntoView: true,
    });
    viewRef.current.focus();
  }, []);

  // Close dropdown on outside click
  useEffect(() => {
    const handleClick = (e: MouseEvent) => {
      if (examplesRef.current && !examplesRef.current.contains(e.target as Node)) {
        setShowExamples(false);
      }
    };
    document.addEventListener('mousedown', handleClick);
    return () => document.removeEventListener('mousedown', handleClick);
  }, []);

  const phases = result?.phases;

  return (
    <div className="flex flex-col h-screen" style={{ background: 'var(--bg-primary)' }}>
      {/* Top Bar */}
      <header
        className="flex items-center justify-between px-4 h-12 shrink-0"
        style={{ borderBottom: '1px solid var(--border)', background: 'var(--bg-secondary)' }}
      >
        <div className="flex items-center gap-3">
          <div className="flex items-center gap-2">
            <div
              className="w-6 h-6 rounded flex items-center justify-center text-xs font-bold"
              style={{
                background: 'linear-gradient(135deg, var(--accent-start), var(--accent-end))',
                color: '#fff',
              }}
            >
              M
            </div>
            <span className="font-semibold text-sm" style={{ color: 'var(--text-primary)' }}>
              MiniLang
            </span>
            <span className="text-xs" style={{ color: 'var(--text-muted)' }}>
              v1.0
            </span>
          </div>

          {/* Examples Dropdown */}
          <div className="relative" ref={examplesRef}>
            <button
              onClick={() => setShowExamples(!showExamples)}
              className="flex items-center gap-1.5 px-2.5 py-1 rounded text-xs transition-colors duration-150"
              style={{
                border: '1px solid var(--border)',
                color: 'var(--text-secondary)',
                background: 'var(--bg-tertiary)',
              }}
              onMouseEnter={(e) => (e.currentTarget.style.borderColor = 'var(--accent)')}
              onMouseLeave={(e) => (e.currentTarget.style.borderColor = 'var(--border)')}
            >
              Examples
              <ChevronDown size={12} />
            </button>

            {showExamples && (
              <div
                className="absolute top-full left-0 mt-1 py-1 rounded-lg shadow-xl z-50 min-w-[220px]"
                style={{ background: 'var(--bg-elevated)', border: '1px solid var(--border)' }}
              >
                {EXAMPLES.map((ex, i) => (
                  <button
                    key={i}
                    onClick={() => selectExample(i)}
                    className="w-full text-left px-3 py-2 text-xs transition-colors duration-100 flex flex-col gap-0.5"
                    style={{ color: 'var(--text-primary)' }}
                    onMouseEnter={(e) =>
                      (e.currentTarget.style.background = 'var(--accent-dim)')
                    }
                    onMouseLeave={(e) =>
                      (e.currentTarget.style.background = 'transparent')
                    }
                  >
                    <span className="font-medium">{ex.name}</span>
                    <span style={{ color: 'var(--text-muted)', fontSize: '10px' }}>
                      {ex.description}
                    </span>
                  </button>
                ))}
              </div>
            )}
          </div>
        </div>

        <div className="flex items-center gap-2">
          {!backendOnline && (
            <div
              className="flex items-center gap-1.5 px-2 py-1 rounded text-xs"
              style={{ background: 'rgba(248,113,113,0.1)', color: 'var(--error)' }}
            >
              <WifiOff size={12} />
              Offline
            </div>
          )}
          <button
            onClick={runCode}
            disabled={loading}
            className="flex items-center gap-1.5 px-3 py-1.5 rounded text-xs font-medium transition-all duration-200"
            style={{
              background: loading
                ? 'var(--bg-tertiary)'
                : 'linear-gradient(135deg, var(--accent-start), var(--accent-end))',
              color: '#fff',
              opacity: loading ? 0.6 : 1,
            }}
          >
            {loading ? <Loader2 size={13} className="animate-spin" /> : <Play size={13} />}
            {loading ? 'Running...' : 'Run'}
            <span style={{ color: 'rgba(255,255,255,0.5)', fontSize: '10px' }}>⌘↵</span>
          </button>
          <button
            onClick={reset}
            className="flex items-center gap-1 px-2 py-1.5 rounded text-xs transition-colors duration-150"
            style={{ color: 'var(--text-muted)', border: '1px solid var(--border)' }}
            onMouseEnter={(e) => (e.currentTarget.style.color = 'var(--text-primary)')}
            onMouseLeave={(e) => (e.currentTarget.style.color = 'var(--text-muted)')}
          >
            <RotateCcw size={12} />
            Reset
          </button>
        </div>
      </header>

      {/* Pipeline Strip */}
      {phases && (
        <div
          className="flex items-center gap-1 px-4 py-1.5 text-xs shrink-0 overflow-x-auto"
          style={{ borderBottom: '1px solid var(--border-subtle)', background: 'var(--bg-secondary)' }}
        >
          <span style={{ color: 'var(--text-muted)' }}>Pipeline:</span>
          {(['lexer', 'parser', 'semantic', 'runtime'] as const).map((phase, i) => {
            const status = phases[phase];
            const colors: Record<string, { bg: string; text: string }> = {
              ok: { bg: 'rgba(52,211,153,0.12)', text: 'var(--success)' },
              error: { bg: 'rgba(248,113,113,0.12)', text: 'var(--error)' },
              skipped: { bg: 'rgba(107,114,128,0.12)', text: 'var(--skipped)' },
            };
            const { bg, text } = colors[status] || colors.skipped;
            return (
              <div key={phase} className="flex items-center gap-1">
                {i > 0 && <span style={{ color: 'var(--text-muted)' }}>→</span>}
                <span
                  className="px-2 py-0.5 rounded font-medium capitalize"
                  style={{ background: bg, color: text }}
                >
                  {phase}: {status}
                </span>
              </div>
            );
          })}
          {result && (
            <span className="ml-auto" style={{ color: 'var(--text-muted)' }}>
              {result.executionTimeMs.toFixed(1)}ms
            </span>
          )}
        </div>
      )}

      {/* Main Content */}
      <div className="flex flex-1 min-h-0 flex-col md:flex-row">
        {/* Editor */}
        <div className="flex-1 min-h-0 flex flex-col" style={{ borderRight: '1px solid var(--border)' }}>
          <div
            className="px-3 py-1.5 text-xs font-medium shrink-0 flex items-center gap-2"
            style={{ borderBottom: '1px solid var(--border-subtle)', color: 'var(--text-muted)' }}
          >
            <Terminal size={12} />
            Editor
          </div>
          <div ref={editorRef} className="flex-1 min-h-0 overflow-auto" style={{ background: 'var(--bg-primary)' }} />
        </div>

        {/* Output Panel */}
        <div className="flex-1 min-h-0 flex flex-col" style={{ maxWidth: '100%' }}>
          <div
            className="px-3 py-1.5 text-xs font-medium shrink-0 flex items-center gap-2"
            style={{ borderBottom: '1px solid var(--border-subtle)', color: 'var(--text-muted)' }}
          >
            <Terminal size={12} />
            Output
          </div>
          <div
            className="flex-1 min-h-0 overflow-auto p-3"
            style={{ background: 'var(--bg-primary)', fontFamily: "'JetBrains Mono', monospace", fontSize: '13px' }}
          >
            {!backendOnline ? (
              <div className="flex flex-col items-center justify-center h-full gap-3" style={{ color: 'var(--text-muted)' }}>
                <WifiOff size={32} />
                <div className="text-sm">Backend is offline</div>
                <div className="text-xs">Start the server: <code style={{ color: 'var(--accent)' }}>npm run dev</code></div>
              </div>
            ) : !result ? (
              <div className="flex flex-col items-center justify-center h-full gap-2" style={{ color: 'var(--text-muted)' }}>
                <Play size={24} />
                <div className="text-xs">Press Run or Ctrl+Enter to execute</div>
              </div>
            ) : result.output.length === 0 && result.errors.length === 0 ? (
              <div className="text-xs" style={{ color: 'var(--text-muted)' }}>
                No output produced.
              </div>
            ) : (
              <div className="flex flex-col gap-0.5">
                {result.output.map((line, i) => (
                  <div key={i} style={{ color: 'var(--text-primary)' }}>
                    {line}
                  </div>
                ))}
                {result.errors.filter(e => e.phase === 'runtime').map((err, i) => (
                  <div key={`err-${i}`} className="mt-1" style={{ color: 'var(--error)' }}>
                    ✗ {err.message}
                  </div>
                ))}
              </div>
            )}
          </div>
        </div>
      </div>

      {/* Bottom Tabs */}
      <div className="shrink-0 flex flex-col" style={{ height: '280px', borderTop: '1px solid var(--border)' }}>
        {/* Tab Headers */}
        <div
          className="flex items-center gap-0.5 px-2 shrink-0"
          style={{ borderBottom: '1px solid var(--border-subtle)', background: 'var(--bg-secondary)' }}
        >
          {TABS.map((tab) => (
            <button
              key={tab.id}
              onClick={() => setActiveTab(tab.id)}
              className="flex items-center gap-1.5 px-3 py-2 text-xs font-medium transition-colors duration-100 relative"
              style={{
                color: activeTab === tab.id ? 'var(--accent)' : 'var(--text-muted)',
              }}
            >
              {tab.icon}
              {tab.label}
              {tab.id === 'errors' && result && result.errors.length > 0 && (
                <span
                  className="px-1.5 py-0 rounded-full text-[10px] font-semibold"
                  style={{ background: 'rgba(248,113,113,0.15)', color: 'var(--error)' }}
                >
                  {result.errors.length}
                </span>
              )}
              {activeTab === tab.id && (
                <div
                  className="absolute bottom-0 left-2 right-2 h-[2px] rounded-full"
                  style={{ background: 'linear-gradient(90deg, var(--accent-start), var(--accent-end))' }}
                />
              )}
            </button>
          ))}
        </div>

        {/* Tab Content */}
        <div className="flex-1 min-h-0 overflow-auto" style={{ background: 'var(--bg-primary)' }}>
          {activeTab === 'tokens' && <TokensTab result={result} />}
          {activeTab === 'ast' && <ASTTab result={result} />}
          {activeTab === 'symbols' && <SymbolsTab result={result} />}
          {activeTab === 'errors' && <ErrorsTab result={result} jumpToLine={jumpToLine} />}
          {activeTab === 'grammar' && <GrammarTab />}
        </div>
      </div>
    </div>
  );
}

// ==================== TOKENS TAB ====================
function TokensTab({ result }: { result: ExecutionResult | null }) {
  if (!result) return <EmptyPanel message="Run code to see tokens" />;

  const tokenColors: Record<string, string> = {
    KEYWORD: 'var(--token-keyword)',
    IDENTIFIER: 'var(--token-identifier)',
    INTEGER: 'var(--token-integer)',
    FLOAT: 'var(--token-float)',
    STRING: 'var(--token-string)',
    OPERATOR: 'var(--token-operator)',
    DELIMITER: 'var(--token-delimiter)',
    NEWLINE: 'var(--token-delimiter)',
    COMMENT: 'var(--token-comment)',
    EOF: 'var(--token-delimiter)',
    ERROR: 'var(--token-error)',
  };

  return (
    <div className="p-2">
      <table className="w-full text-xs" style={{ fontFamily: "'JetBrains Mono', monospace" }}>
        <thead>
          <tr style={{ color: 'var(--text-muted)', borderBottom: '1px solid var(--border-subtle)' }}>
            <th className="text-left p-1.5 font-medium">Index</th>
            <th className="text-left p-1.5 font-medium">Type</th>
            <th className="text-left p-1.5 font-medium">Lexeme</th>
            <th className="text-right p-1.5 font-medium">Line</th>
            <th className="text-right p-1.5 font-medium">Col</th>
          </tr>
        </thead>
        <tbody>
          {result.tokens.map((tok, i) => (
            <tr
              key={i}
              className="transition-colors duration-100"
              style={{ borderBottom: '1px solid var(--border-subtle)' }}
              onMouseEnter={(e) =>
                (e.currentTarget.style.background = 'var(--accent-dim)')
              }
              onMouseLeave={(e) =>
                (e.currentTarget.style.background = 'transparent')
              }
            >
              <td className="p-1.5" style={{ color: 'var(--text-muted)' }}>
                {tok.index}
              </td>
              <td className="p-1.5">
                <span
                  className="px-1.5 py-0.5 rounded text-[10px] font-semibold"
                  style={{
                    background: `${tokenColors[tok.type] || 'var(--text-muted)'}18`,
                    color: tokenColors[tok.type] || 'var(--text-muted)',
                  }}
                >
                  {tok.type}
                </span>
              </td>
              <td className="p-1.5" style={{ color: 'var(--text-primary)' }}>
                {tok.lexeme === '\\n' ? (
                  <span style={{ color: 'var(--text-muted)' }}>↵</span>
                ) : tok.lexeme === '' ? (
                  <span style={{ color: 'var(--text-muted)' }}>eof</span>
                ) : (
                  tok.lexeme
                )}
              </td>
              <td className="p-1.5 text-right" style={{ color: 'var(--text-muted)' }}>
                {tok.line}
              </td>
              <td className="p-1.5 text-right" style={{ color: 'var(--text-muted)' }}>
                {tok.col}
              </td>
            </tr>
          ))}
        </tbody>
      </table>
    </div>
  );
}

// ==================== AST TAB ====================
function ASTTab({ result }: { result: ExecutionResult | null }) {
  if (!result || !result.ast) return <EmptyPanel message="Run code to see AST" />;

  return (
    <div className="p-3" style={{ fontFamily: "'JetBrains Mono', monospace", fontSize: '12px' }}>
      <ASTNodeView node={result.ast} depth={0} />
    </div>
  );
}

function ASTNodeView({ node, depth }: { node: Record<string, unknown>; depth: number }) {
  const [expanded, setExpanded] = useState(depth < 3);

  if (!node || typeof node !== 'object') return null;

  const nodeType = node.type as string;
  const line = node.line as number;
  const col = node.col as number;

  const nodeColors: Record<string, string> = {
    Program: '#818cf8',
    Assign: '#c084fc',
    Print: '#34d399',
    If: '#fbbf24',
    While: '#f59e0b',
    BinaryExpr: '#f87171',
    UnaryExpr: '#fb923c',
    Literal: '#60a5fa',
    Variable: '#93c5fd',
  };

  const childKeys = Object.keys(node).filter(
    (k) =>
      k !== 'type' && k !== 'line' && k !== 'col' && k !== 'valueType' &&
      (typeof node[k] === 'object' || k === 'name' || k === 'op' || k === 'value')
  );

  return (
    <div style={{ marginLeft: depth > 0 ? 16 : 0 }}>
      <div
        className="flex items-center gap-1 py-0.5 cursor-pointer select-none"
        onClick={() => setExpanded(!expanded)}
      >
        <span style={{ color: 'var(--text-muted)', width: '12px', fontSize: '10px' }}>
          {childKeys.length > 0 ? (expanded ? '▼' : '▶') : ' '}
        </span>
        <span
          className="px-1.5 py-0.5 rounded text-[10px] font-semibold"
          style={{
            background: `${nodeColors[nodeType] || '#6b7280'}20`,
            color: nodeColors[nodeType] || '#6b7280',
          }}
        >
          {nodeType}
        </span>
        {typeof node.name === 'string' && node.name && (
          <span style={{ color: 'var(--token-identifier)' }}>
            {node.name}
          </span>
        )}
        {typeof node.op === 'string' && node.op && (
          <span style={{ color: 'var(--token-operator)' }}>
            {node.op}
          </span>
        )}
        {nodeType === 'Literal' && (
          <span style={{ color: 'var(--token-string)' }}>
            {String(node.value)} <span style={{ color: 'var(--text-muted)', fontSize: '10px' }}>({node.valueType as string})</span>
          </span>
        )}
        <span style={{ color: 'var(--text-muted)', fontSize: '10px', marginLeft: 'auto' }}>
          {line}:{col}
        </span>
      </div>

      {expanded &&
        childKeys.map((key) => {
          const val = node[key];
          if (Array.isArray(val)) {
            return (
              <div key={key} style={{ marginLeft: 16 }}>
                <div className="py-0.5" style={{ color: 'var(--text-muted)', fontSize: '10px' }}>
                  {key}:
                </div>
                {val.map((child: Record<string, unknown>, i: number) => (
                  <ASTNodeView key={i} node={child} depth={depth + 1} />
                ))}
                {val.length === 0 && (
                  <span className="ml-4" style={{ color: 'var(--text-muted)', fontSize: '10px' }}>
                    (empty)
                  </span>
                )}
              </div>
            );
          }
          if (val && typeof val === 'object' && 'type' in (val as Record<string, unknown>)) {
            return (
              <div key={key} style={{ marginLeft: 16 }}>
                <div className="py-0.5" style={{ color: 'var(--text-muted)', fontSize: '10px' }}>
                  {key}:
                </div>
                <ASTNodeView node={val as Record<string, unknown>} depth={depth + 1} />
              </div>
            );
          }
          return null;
        })}
    </div>
  );
}

// ==================== SYMBOLS TAB ====================
function SymbolsTab({ result }: { result: ExecutionResult | null }) {
  if (!result) return <EmptyPanel message="Run code to see symbol table" />;
  if (result.symbols.length === 0)
    return <EmptyPanel message="No variables in scope" />;

  return (
    <div className="p-2">
      <table className="w-full text-xs" style={{ fontFamily: "'JetBrains Mono', monospace" }}>
        <thead>
          <tr style={{ color: 'var(--text-muted)', borderBottom: '1px solid var(--border-subtle)' }}>
            <th className="text-left p-1.5 font-medium">Name</th>
            <th className="text-left p-1.5 font-medium">Type</th>
            <th className="text-left p-1.5 font-medium">Value</th>
            <th className="text-left p-1.5 font-medium">Scope</th>
          </tr>
        </thead>
        <tbody>
          {result.symbols.map((sym, i) => (
            <tr
              key={i}
              style={{ borderBottom: '1px solid var(--border-subtle)' }}
              className="transition-colors duration-100"
              onMouseEnter={(e) =>
                (e.currentTarget.style.background = 'var(--accent-dim)')
              }
              onMouseLeave={(e) =>
                (e.currentTarget.style.background = 'transparent')
              }
            >
              <td className="p-1.5" style={{ color: 'var(--token-identifier)' }}>
                {sym.name}
              </td>
              <td className="p-1.5">
                <span
                  className="px-1.5 py-0.5 rounded text-[10px] font-semibold"
                  style={{
                    background: 'rgba(96,165,250,0.12)',
                    color: '#60a5fa',
                  }}
                >
                  {sym.type}
                </span>
              </td>
              <td className="p-1.5" style={{ color: 'var(--text-primary)' }}>
                {sym.value}
              </td>
              <td className="p-1.5" style={{ color: 'var(--text-muted)' }}>
                {sym.scope}
              </td>
            </tr>
          ))}
        </tbody>
      </table>
    </div>
  );
}

// ==================== ERRORS TAB ====================
function ErrorsTab({
  result,
  jumpToLine,
}: {
  result: ExecutionResult | null;
  jumpToLine: (line: number) => void;
}) {
  if (!result) return <EmptyPanel message="Run code to see errors" />;
  if (result.errors.length === 0)
    return (
      <div className="flex items-center justify-center h-full">
        <div className="flex items-center gap-2 text-xs" style={{ color: 'var(--success)' }}>
          ✓ No errors
        </div>
      </div>
    );

  const grouped: Record<string, ErrorEntry[]> = {};
  for (const err of result.errors) {
    if (!grouped[err.phase]) grouped[err.phase] = [];
    grouped[err.phase].push(err);
  }

  return (
    <div className="p-3 flex flex-col gap-3">
      {Object.entries(grouped).map(([phase, errors]) => (
        <div key={phase}>
          <div className="flex items-center gap-2 mb-1.5">
            <span
              className="px-2 py-0.5 rounded text-[10px] font-semibold uppercase"
              style={{ background: 'rgba(248,113,113,0.12)', color: 'var(--error)' }}
            >
              {phase}
            </span>
            <span className="text-[10px]" style={{ color: 'var(--text-muted)' }}>
              {errors.length} error{errors.length > 1 ? 's' : ''}
            </span>
          </div>
          {errors.map((err, i) => (
            <div
              key={i}
              className="flex items-start gap-2 py-1.5 px-2 rounded text-xs cursor-pointer transition-colors duration-100"
              style={{ color: 'var(--text-primary)' }}
              onClick={() => jumpToLine(err.line)}
              onMouseEnter={(e) =>
                (e.currentTarget.style.background = 'rgba(248,113,113,0.06)')
              }
              onMouseLeave={(e) =>
                (e.currentTarget.style.background = 'transparent')
              }
            >
              <AlertTriangle size={12} style={{ color: 'var(--error)', marginTop: '1px', flexShrink: 0 }} />
              <div className="flex flex-col gap-0.5">
                <span>{err.message}</span>
                <span style={{ color: 'var(--text-muted)', fontSize: '10px' }}>
                  {err.type} — Line {err.line}, Col {err.col}
                </span>
              </div>
            </div>
          ))}
        </div>
      ))}
    </div>
  );
}

// ==================== GRAMMAR TAB ====================
function GrammarTab() {
  return (
    <div className="p-4 overflow-auto text-xs" style={{ fontFamily: "'JetBrains Mono', monospace" }}>
      <div className="flex flex-col md:flex-row gap-6">
        {/* CFG */}
        <div className="flex-1">
          <h3
            className="text-xs font-semibold mb-2 uppercase tracking-wider"
            style={{ color: 'var(--accent)' }}
          >
            Context-Free Grammar
          </h3>
          <div className="flex flex-col gap-1" style={{ color: 'var(--text-secondary)' }}>
            <GrammarRule rule="program" def="statement*" />
            <GrammarRule rule="statement" def="SET ID '=' expr NL" />
            <GrammarRule rule="" def="| PRINT expr NL" />
            <GrammarRule rule="" def="| IF expr NL statement* (ELSE NL statement*)? END NL" />
            <GrammarRule rule="" def="| WHILE expr NL statement* END NL" />
            <div className="my-1" style={{ borderTop: '1px solid var(--border-subtle)' }} />
            <GrammarRule rule="expr" def="or" />
            <GrammarRule rule="or" def="and (OR and)*" />
            <GrammarRule rule="and" def="not (AND not)*" />
            <GrammarRule rule="not" def="NOT not | comparison" />
            <GrammarRule rule="comparison" def="arith ((> | < | >= | <= | == | !=) arith)*" />
            <GrammarRule rule="arith" def="term ((+ | -) term)*" />
            <GrammarRule rule="term" def="unary ((* | / | %) unary)*" />
            <GrammarRule rule="unary" def="'-' unary | factor" />
            <GrammarRule rule="factor" def="INT | FLOAT | STRING | TRUE | FALSE | ID | '(' expr ')'" />
          </div>
        </div>

        <div className="flex flex-col gap-4">
          {/* Precedence */}
          <div>
            <h3
              className="text-xs font-semibold mb-2 uppercase tracking-wider"
              style={{ color: 'var(--accent)' }}
            >
              Operator Precedence (high → low)
            </h3>
            <table className="w-full">
              <thead>
                <tr style={{ color: 'var(--text-muted)', borderBottom: '1px solid var(--border-subtle)' }}>
                  <th className="text-left p-1 font-medium">Level</th>
                  <th className="text-left p-1 font-medium">Operators</th>
                  <th className="text-left p-1 font-medium">Assoc</th>
                </tr>
              </thead>
              <tbody style={{ color: 'var(--text-secondary)' }}>
                {[
                  { level: '1', ops: '( )', assoc: '—' },
                  { level: '2', ops: 'unary -', assoc: 'right' },
                  { level: '3', ops: '* / %', assoc: 'left' },
                  { level: '4', ops: '+ -', assoc: 'left' },
                  { level: '5', ops: '> < >= <= == !=', assoc: 'left' },
                  { level: '6', ops: 'NOT', assoc: 'right' },
                  { level: '7', ops: 'AND', assoc: 'left' },
                  { level: '8', ops: 'OR', assoc: 'left' },
                ].map((row) => (
                  <tr key={row.level} style={{ borderBottom: '1px solid var(--border-subtle)' }}>
                    <td className="p-1" style={{ color: 'var(--text-muted)' }}>{row.level}</td>
                    <td className="p-1" style={{ color: 'var(--token-operator)' }}>{row.ops}</td>
                    <td className="p-1" style={{ color: 'var(--text-muted)' }}>{row.assoc}</td>
                  </tr>
                ))}
              </tbody>
            </table>
          </div>

          {/* Regex Spec */}
          <div>
            <h3
              className="text-xs font-semibold mb-2 uppercase tracking-wider"
              style={{ color: 'var(--accent)' }}
            >
              Lexer Regex Specification
            </h3>
            <div className="flex flex-col gap-1" style={{ color: 'var(--text-secondary)' }}>
              {[
                { token: 'IDENTIFIER', regex: '[A-Za-z_][A-Za-z0-9_]*' },
                { token: 'INTEGER', regex: '[0-9]+' },
                { token: 'FLOAT', regex: '[0-9]+\\.[0-9]+' },
                { token: 'STRING', regex: '"[^"\\n]*"' },
                { token: 'COMMENT', regex: '#.*' },
              ].map((spec) => (
                <div key={spec.token} className="flex gap-3 items-center">
                  <span className="w-24" style={{ color: 'var(--text-muted)' }}>
                    {spec.token}
                  </span>
                  <code style={{ color: 'var(--accent)' }}>{spec.regex}</code>
                </div>
              ))}
            </div>
          </div>
        </div>
      </div>
    </div>
  );
}

function GrammarRule({ rule, def }: { rule: string; def: string }) {
  return (
    <div className="flex gap-2">
      <span className="w-24 text-right shrink-0" style={{ color: 'var(--token-keyword)' }}>
        {rule}
      </span>
      <span style={{ color: 'var(--text-muted)' }}>{rule ? '→' : ' '}</span>
      <span>{def}</span>
    </div>
  );
}

// ==================== EMPTY PANEL ====================
function EmptyPanel({ message }: { message: string }) {
  return (
    <div className="flex items-center justify-center h-full">
      <span className="text-xs" style={{ color: 'var(--text-muted)' }}>
        {message}
      </span>
    </div>
  );
}

export default App;
