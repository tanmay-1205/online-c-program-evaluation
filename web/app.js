const API_BASE = '/api';

let currentProblem = null;
let pollTimer = null;
let authMode = 'login';
let languages = [];
let templates = {};

const CODE_TEMPLATES = {
  c: {
    sum_two: `#include <stdio.h>

int main() {
    int a, b;
    scanf("%d %d", &a, &b);
    printf("%d\\n", a + b);
    return 0;
}`,
    fibonacci: `#include <stdio.h>

int main() {
    int n;
    scanf("%d", &n);
    if (n <= 1) { printf("%d\\n", n); return 0; }
    long long a = 0, b = 1;
    for (int i = 2; i <= n; i++) {
        long long c = a + b;
        a = b; b = c;
    }
    printf("%lld\\n", b);
    return 0;
}`,
    stress_120: `#include <stdio.h>

int main() {
    int a, b;
    scanf("%d %d", &a, &b);
    printf("%d\\n", a * b + a - b);
    return 0;
}`
  },
  cpp: {
    sum_two: `#include <iostream>
using namespace std;

int main() {
    int a, b;
    cin >> a >> b;
    cout << a + b << "\\n";
    return 0;
}`,
    fibonacci: `#include <iostream>
using namespace std;

int main() {
    int n;
    cin >> n;
    if (n <= 1) { cout << n << "\\n"; return 0; }
    long long a = 0, b = 1;
    for (int i = 2; i <= n; i++) {
        long long c = a + b;
        a = b; b = c;
    }
    cout << b << "\\n";
    return 0;
}`,
    stress_120: `#include <iostream>
using namespace std;

int main() {
    int a, b;
    cin >> a >> b;
    cout << a * b + a - b << "\\n";
    return 0;
}`
  },
  python: {
    sum_two: `a, b = map(int, input().split())
print(a + b)`,
    fibonacci: `n = int(input())
if n <= 1:
    print(n)
else:
    a, b = 0, 1
    for _ in range(2, n + 1):
        a, b = b, a + b
    print(b)`,
    stress_120: `a, b = map(int, input().split())
print(a * b + a - b)`
  }
};

function getToken() {
  return localStorage.getItem('ocpe_token');
}

function getUsername() {
  return localStorage.getItem('ocpe_username');
}

function authHeaders() {
  const headers = { 'Content-Type': 'application/json' };
  const token = getToken();
  if (token) headers['Authorization'] = `Bearer ${token}`;
  return headers;
}

function updateAuthUI() {
  const loggedIn = !!getToken();
  document.getElementById('login-btn').style.display = loggedIn ? 'none' : 'inline-block';
  document.getElementById('register-btn').style.display = loggedIn ? 'none' : 'inline-block';
  document.getElementById('logout-btn').style.display = loggedIn ? 'inline-block' : 'none';
  document.getElementById('history-btn').style.display = loggedIn ? 'inline-block' : 'none';
  document.getElementById('user-label').style.display = loggedIn ? 'inline' : 'none';
  document.getElementById('user-label').textContent = loggedIn ? getUsername() : '';
}

function logout() {
  localStorage.removeItem('ocpe_token');
  localStorage.removeItem('ocpe_username');
  updateAuthUI();
  showProblems();
}

function showAuth(mode) {
  authMode = mode;
  document.getElementById('auth-title').textContent = mode === 'login' ? 'Login' : 'Register';
  document.getElementById('auth-submit-btn').textContent = mode === 'login' ? 'Login' : 'Register';
  document.getElementById('auth-error').textContent = '';
  document.getElementById('auth-section').style.display = 'block';
  document.getElementById('problems-section').style.display = 'none';
  document.getElementById('editor-section').style.display = 'none';
  document.getElementById('result-section').style.display = 'none';
  document.getElementById('history-section').style.display = 'none';
}

function hideAuth() {
  document.getElementById('auth-section').style.display = 'none';
  document.getElementById('problems-section').style.display = 'block';
}

async function submitAuth() {
  const username = document.getElementById('auth-username').value.trim();
  const password = document.getElementById('auth-password').value;
  const endpoint = authMode === 'login' ? '/auth/login' : '/auth/register';

  try {
    const res = await fetch(`${API_BASE}${endpoint}`, {
      method: 'POST',
      headers: { 'Content-Type': 'application/json' },
      body: JSON.stringify({ username, password })
    });
    const data = await res.json();
    if (!res.ok) {
      document.getElementById('auth-error').textContent = data.error || 'Authentication failed';
      return;
    }
    localStorage.setItem('ocpe_token', data.token);
    localStorage.setItem('ocpe_username', data.username);
    updateAuthUI();
    hideAuth();
  } catch (e) {
    document.getElementById('auth-error').textContent = 'Connection failed';
  }
}

async function loadLanguages() {
  try {
    const res = await fetch(`${API_BASE}/languages`);
    languages = await res.json();
    const select = document.getElementById('language-select');
    select.innerHTML = languages.map(l =>
      `<option value="${l.id}">${l.name}</option>`
    ).join('');
    select.onchange = () => applyTemplate();
  } catch (e) {
    languages = [{ id: 'c', name: 'C' }];
  }
}

function applyTemplate() {
  if (!currentProblem) return;
  const lang = document.getElementById('language-select').value;
  const tpl = (CODE_TEMPLATES[lang] && CODE_TEMPLATES[lang][currentProblem.id])
    || (lang === 'python' ? 'print("Hello")\n' : '#include <stdio.h>\n\nint main() {\n    return 0;\n}');
  document.getElementById('source-code').value = tpl;
}

async function loadProblems() {
  const list = document.getElementById('problems-list');
  try {
    const res = await fetch(`${API_BASE}/problems`);
    const problems = await res.json();
    if (!problems.length) {
      list.innerHTML = '<p>No problems available.</p>';
      return;
    }
    list.innerHTML = problems.map(p => `
      <div class="problem-item" onclick="openProblem('${p.id}')">
        <span><strong>${p.title}</strong></span>
        <span class="badge">${p.test_case_count} tests · ${p.time_limit_ms}ms · ${p.memory_limit_mb}MB</span>
      </div>
    `).join('');
  } catch (e) {
    list.innerHTML = '<p>Failed to load problems. Is the API running?</p>';
  }
}

async function openProblem(id) {
  if (!getToken()) {
    alert('Please login to solve problems');
    showAuth('login');
    return;
  }
  try {
    const res = await fetch(`${API_BASE}/problems/${id}`);
    currentProblem = await res.json();
    document.getElementById('problem-title').textContent = currentProblem.title;
    document.getElementById('problem-description').textContent = currentProblem.description;
    document.getElementById('time-limit').textContent = `Time: ${currentProblem.time_limit_ms}ms`;
    document.getElementById('memory-limit').textContent = `Memory: ${currentProblem.memory_limit_mb}MB`;
    document.getElementById('test-count').textContent = `Tests: ${currentProblem.test_case_count}`;
    applyTemplate();
    showEditor();
  } catch (e) {
    alert('Failed to load problem');
  }
}

async function showHistory() {
  if (!getToken()) { showAuth('login'); return; }
  document.getElementById('problems-section').style.display = 'none';
  document.getElementById('editor-section').style.display = 'none';
  document.getElementById('result-section').style.display = 'none';
  document.getElementById('auth-section').style.display = 'none';
  document.getElementById('history-section').style.display = 'block';

  const list = document.getElementById('history-list');
  list.innerHTML = 'Loading...';
  try {
    const res = await fetch(`${API_BASE}/auth/submissions`, { headers: authHeaders() });
    const subs = await res.json();
    if (!subs.length) {
      list.innerHTML = '<p>No submissions yet.</p>';
      return;
    }
    list.innerHTML = subs.map(s => `
      <div class="history-item" onclick="viewSubmission('${s.submission_id}')">
        <span>${s.problem_id || 'unknown'} · ${s.language || 'c'}</span>
        <span class="verdict ${s.verdict}" style="font-size:0.85rem">${s.verdict || 'PENDING'}</span>
      </div>
    `).join('');
  } catch (e) {
    list.innerHTML = '<p>Failed to load submissions.</p>';
  }
}

function viewSubmission(id) {
  showResult();
  pollSubmission(id);
}

function showProblems() {
  document.getElementById('problems-section').style.display = 'block';
  document.getElementById('editor-section').style.display = 'none';
  document.getElementById('result-section').style.display = 'none';
  document.getElementById('history-section').style.display = 'none';
  document.getElementById('auth-section').style.display = 'none';
  if (pollTimer) clearInterval(pollTimer);
}

function showEditor() {
  document.getElementById('problems-section').style.display = 'none';
  document.getElementById('editor-section').style.display = 'block';
  document.getElementById('result-section').style.display = 'none';
  document.getElementById('history-section').style.display = 'none';
  document.getElementById('auth-section').style.display = 'none';
  if (pollTimer) clearInterval(pollTimer);
}

function showResult() {
  document.getElementById('problems-section').style.display = 'none';
  document.getElementById('editor-section').style.display = 'none';
  document.getElementById('result-section').style.display = 'block';
  document.getElementById('history-section').style.display = 'none';
  document.getElementById('auth-section').style.display = 'none';
}

async function submitCode() {
  if (!getToken()) { showAuth('login'); return; }

  const btn = document.getElementById('submit-btn');
  btn.disabled = true;
  btn.textContent = 'Submitting...';

  try {
    const res = await fetch(`${API_BASE}/submit`, {
      method: 'POST',
      headers: authHeaders(),
      body: JSON.stringify({
        problem_id: currentProblem.id,
        source_code: document.getElementById('source-code').value,
        language: document.getElementById('language-select').value
      })
    });
    const data = await res.json();
    if (!res.ok) {
      alert(data.error || 'Submit failed');
      return;
    }
    showResult();
    pollSubmission(data.submission_id);
  } catch (e) {
    alert('Submit failed');
  } finally {
    btn.disabled = false;
    btn.textContent = 'Submit';
  }
}

function pollSubmission(id) {
  const summary = document.getElementById('result-summary');
  summary.innerHTML = '<p class="verdict PENDING">PENDING...</p>';

  if (pollTimer) clearInterval(pollTimer);
  pollTimer = setInterval(async () => {
    try {
      const res = await fetch(`${API_BASE}/submissions/${id}`, { headers: authHeaders() });
      const data = await res.json();
      renderResult(data);
      const terminal = ['ACCEPTED', 'WRONG_ANSWER', 'TIME_LIMIT_EXCEEDED',
        'MEMORY_LIMIT_EXCEEDED', 'RUNTIME_ERROR', 'COMPILATION_ERROR', 'INTERNAL_ERROR'];
      if (terminal.includes(data.verdict)) clearInterval(pollTimer);
    } catch (e) { /* retry */ }
  }, 1000);
}

function renderResult(data) {
  const summary = document.getElementById('result-summary');
  const details = document.getElementById('result-details');
  const v = data.verdict || 'PENDING';

  let html = `<p class="verdict ${v}">${v}</p>`;
  if (data.language) html += `<p style="color:var(--muted);font-size:0.875rem">Language: ${data.language}</p>`;

  if (data.passed !== undefined) {
    html += `<div class="stats">
      <div class="stat-box"><div class="value">${data.passed}/${data.total}</div><div class="label">Passed</div></div>
      <div class="stat-box"><div class="value">${data.max_time_ms || 0}ms</div><div class="label">Max Time</div></div>
      <div class="stat-box"><div class="value">${data.max_memory_kb || 0}KB</div><div class="label">Peak Memory</div></div>
    </div>`;
  }

  if (data.compile_output) {
    html += `<pre style="background:var(--bg);padding:1rem;border-radius:6px;font-size:0.8rem;overflow:auto">${escapeHtml(data.compile_output)}</pre>`;
  }
  summary.innerHTML = html;

  if (data.test_results && data.test_results.length) {
    const show = data.test_results.length > 20
      ? data.test_results.slice(0, 10).concat(data.test_results.slice(-5))
      : data.test_results;
    details.innerHTML = `<h3>Test Cases${data.test_results.length > 20 ? ` (showing 15 of ${data.test_results.length})` : ''}</h3>` +
      show.map(tc => `
      <div class="test-case">
        <div class="tc-header">
          <span>Test #${tc.index + 1}</span>
          <span class="verdict ${tc.verdict}" style="font-size:0.85rem">${tc.verdict}</span>
        </div>
        <div>Time: ${tc.time_ms}ms · Memory: ${tc.memory_kb}KB</div>
        ${tc.verdict !== 'ACCEPTED' ? `<div style="margin-top:0.5rem;color:var(--muted)">${escapeHtml(tc.error_message || '')}</div>` : ''}
      </div>
    `).join('');
  } else {
    details.innerHTML = '';
  }
}

function escapeHtml(s) {
  const d = document.createElement('div');
  d.textContent = s;
  return d.innerHTML;
}

updateAuthUI();
loadLanguages();
loadProblems();
