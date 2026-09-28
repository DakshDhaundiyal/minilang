import { ExecutionResult } from './types';

const API_URL = 'http://localhost:3001';

export async function executeCode(source: string): Promise<ExecutionResult> {
  const response = await fetch(`${API_URL}/api/execute`, {
    method: 'POST',
    headers: { 'Content-Type': 'application/json' },
    body: JSON.stringify({ source }),
  });

  if (!response.ok && response.status !== 408 && response.status !== 413) {
    throw new Error(`API error: ${response.status} ${response.statusText}`);
  }

  return response.json();
}

export async function healthCheck(): Promise<boolean> {
  try {
    const response = await fetch(`${API_URL}/api/health`);
    return response.ok;
  } catch {
    return false;
  }
}
