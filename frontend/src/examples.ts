export interface Example {
  name: string;
  description: string;
  code: string;
}

export const EXAMPLES: Example[] = [
  {
    name: '👋 Hello World',
    description: 'Simple output',
    code: `# Hello World in MiniLang
PRINT "Hello, World!"
PRINT "Welcome to MiniLang!"
`,
  },
  {
    name: '🔢 Arithmetic',
    description: 'Math operations',
    code: `# Arithmetic operations
SET a = 10
SET b = 3

PRINT a + b
PRINT a - b
PRINT a * b
PRINT a / b
PRINT a % b

# Float arithmetic
SET pi = 3.14159
SET radius = 5.0
SET area = pi * radius * radius
PRINT area
`,
  },
  {
    name: '🔀 Conditions',
    description: 'IF/ELSE branching',
    code: `# Conditional statements
SET age = 20
SET hasLicense = TRUE

IF age >= 18 AND hasLicense
  PRINT "You can drive!"
ELSE
  PRINT "You cannot drive."
END

# Nested conditions
SET score = 85
IF score >= 90
  PRINT "Grade: A"
ELSE
  IF score >= 80
    PRINT "Grade: B"
  ELSE
    PRINT "Grade: C"
  END
END
`,
  },
  {
    name: '🔁 Loop',
    description: 'WHILE loop with counter',
    code: `# Sum of 1 to 10
SET sum = 0
SET i = 1

WHILE i <= 10
  SET sum = sum + i
  SET i = i + 1
END

PRINT "Sum of 1 to 10:"
PRINT sum

# Countdown
SET n = 5
WHILE n > 0
  PRINT n
  SET n = n - 1
END
PRINT "Liftoff!"
`,
  },
  {
    name: '⚠️ Undefined Variable',
    description: 'Semantic error example',
    code: `# Using a variable that was never declared
PRINT x
PRINT "This won't execute"
`,
  },
  {
    name: '♾️ Infinite Loop',
    description: 'Execution limit hit',
    code: `# This loop never terminates
# The interpreter will stop it after 100,000 iterations
SET x = TRUE
WHILE x
  SET x = TRUE
END
`,
  },
  {
    name: '🔤 Lexical Error',
    description: 'Invalid characters & malformed tokens',
    code: `# Lexical errors - invalid characters
SET x = 42
PRINT @invalid
SET y = 12abc
PRINT "unterminated string
`,
  },
  {
    name: '🏗️ Syntax Error',
    description: 'Missing END keyword',
    code: `# Syntax error - missing END
IF TRUE
  PRINT "inside if"
  IF FALSE
    PRINT "nested"
  END
`,
  },
];
