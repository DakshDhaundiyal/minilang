import {
  StreamLanguage,
  StringStream,
} from '@codemirror/language';

const KEYWORDS = new Set([
  'SET', 'PRINT', 'IF', 'ELSE', 'END', 'WHILE',
  'AND', 'OR', 'NOT', 'TRUE', 'FALSE',
]);

interface MiniLangState {
  inString: boolean;
}

const miniLangStreamParser = {
  startState(): MiniLangState {
    return { inString: false };
  },

  token(stream: StringStream, _state: MiniLangState): string | null {
    // Skip whitespace
    if (stream.eatSpace()) return null;

    // Comment
    if (stream.match('#')) {
      stream.skipToEnd();
      return 'cm-minilang-comment';
    }

    // String
    if (stream.match('"')) {
      while (!stream.eol()) {
        if (stream.next() === '"') break;
      }
      return 'cm-minilang-string';
    }

    // Numbers
    if (stream.match(/^[0-9]+\.[0-9]+/)) return 'cm-minilang-number';
    if (stream.match(/^[0-9]+/)) return 'cm-minilang-number';

    // Operators (multi-char first)
    if (stream.match('>=') || stream.match('<=') || stream.match('==') || stream.match('!=')) {
      return 'cm-minilang-operator';
    }
    if (stream.match(/^[+\-*/%><]/)) return 'cm-minilang-operator';

    // Delimiters
    if (stream.match('=')) return 'cm-minilang-operator';
    if (stream.match('(') || stream.match(')')) return 'cm-minilang-operator';

    // Identifiers and keywords
    if (stream.match(/^[A-Za-z_][A-Za-z0-9_]*/)) {
      const word = stream.current();
      if (word === 'TRUE' || word === 'FALSE') return 'cm-minilang-boolean';
      if (KEYWORDS.has(word)) return 'cm-minilang-keyword';
      return 'cm-minilang-identifier';
    }

    // Skip unknown characters
    stream.next();
    return null;
  },
};

export const miniLangLanguage = StreamLanguage.define(miniLangStreamParser);
