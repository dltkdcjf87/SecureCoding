import { useState, useRef, useEffect } from 'react'
import './index.css'

export interface Issue {
  line: number;
  vulnerability: string;
  severity: 'Critical' | 'High' | 'Medium' | 'Low';
  description: string;
  original_code: string;
  suggested_code: string;
  action_type: string;
}

export interface FileAudit {
  file_path: string;
  issues: Issue[];
}

function App() {
  const [audits, setAudits] = useState<FileAudit[]>([]);
  const [selectedFile, setSelectedFile] = useState<FileAudit | null>(null);
  const [isAuditing, setIsAuditing] = useState(false);
  const [auditProgress, setAuditProgress] = useState({ current: 0, total: 0 });
  const [showChecklistModal, setShowChecklistModal] = useState(false);
  const folderInputRef = useRef<HTMLInputElement>(null);

  const stats = {
    total: audits.reduce((acc, curr) => acc + curr.issues.length, 0),
    critical: audits.reduce((acc, curr) => acc + curr.issues.filter(i => i.severity === 'Critical').length, 0),
    high: audits.reduce((acc, curr) => acc + curr.issues.filter(i => i.severity === 'High').length, 0),
    medium: audits.reduce((acc, curr) => acc + curr.issues.filter(i => i.severity === 'Medium').length, 0),
  };

  const handleFolderSelect = async (event: React.ChangeEvent<HTMLInputElement>) => {
    const files = event.target.files;
    if (!files || files.length === 0) return;

    // Filter supported files
    const supportedExtensions = ['.py', '.js', '.ts', '.tsx', '.java', '.jsp', '.html', '.css', '.c', '.cpp', '.cc', '.cxx', '.h', '.hpp', '.hxx', '.php'];

    console.log(`[File Selection] Total files found in folder: ${files.length}`);

    const validFiles = Array.from(files).filter(file => {
      const fileName = file.name.toLowerCase();
      const relativePath = file.webkitRelativePath || '';

      const isSupported = supportedExtensions.some(ext => fileName.endsWith(ext));
      const isIgnored =
        relativePath.includes('node_modules') ||
        relativePath.includes('.venv') ||
        relativePath.includes('.git') ||
        relativePath.includes('dist') ||
        relativePath.includes('build');

      if (!isSupported) {
        // console.debug(`[Filtered] Unsupported extension: ${fileName}`);
      } else if (isIgnored) {
        console.debug(`[Filtered] Ignored path: ${relativePath}`);
      }

      return isSupported && !isIgnored;
    });

    console.log(`[File Selection] Filtered to ${validFiles.length} valid source files.`);
    if (validFiles.length > 0) {
      console.log('Sample files:', validFiles.slice(0, 5).map(f => f.webkitRelativePath || f.name));
    }

    if (validFiles.length === 0) {
      const sampleFile = files[0];
      alert(`선택한 폴더에 지원되는 소스 파일이 없습니다. (총 ${files.length}개 파일 탐색됨)\n첫 번째 파일 예시: ${sampleFile.webkitRelativePath || sampleFile.name}`);
      return;
    }

    setIsAuditing(true);
    setAuditProgress({ current: 0, total: validFiles.length });
    setAudits([]); // Clear previous results for a fresh start
    setSelectedFile(null);

    const results: FileAudit[] = [];

    for (let i = 0; i < validFiles.length; i++) {
      const file = validFiles[i];
      setAuditProgress(prev => ({ ...prev, current: i + 1 }));

      const formData = new FormData();
      formData.append('file', file, file.webkitRelativePath || file.name);

      try {
        const response = await fetch('http://localhost:8001/api/audit', {
          method: 'POST',
          body: formData,
        });

        if (response.ok) {
          const result: FileAudit = await response.json();
          results.push(result);
          setAudits(prev => [...prev, result]);
          if (results.length === 1) setSelectedFile(result);
        }
      } catch (error) {
        console.error(`Error auditing file ${file.name}:`, error);
      }
    }

    setIsAuditing(false);
    if (folderInputRef.current) folderInputRef.current.value = '';
  };

  return (
    <div className="container">
      <header className="flex-between" style={{ marginBottom: '3rem' }}>
        <div>
          <h1 className="gradient-text" style={{ fontSize: '2.5rem', marginBottom: '0.5rem' }}>보안 취약점 감사 대시보드</h1>
          <p style={{ color: 'var(--text-muted)' }}>프로젝트 폴더를 선택하여 전체 소스 코드를 한꺼번에 점검하세요.</p>
        </div>
        <div>
          <input
            type="file"
            ref={folderInputRef}
            onChange={handleFolderSelect}
            style={{ display: 'none' }}
            {...({ webkitdirectory: "", directory: "" } as any)}
          />
          <button
            className="btn-primary"
            onClick={() => folderInputRef.current?.click()}
            disabled={isAuditing}
            style={{ padding: '15px 30px', fontSize: '1.1rem' }}
          >
            {isAuditing ? (
              <>
                <svg className="animate-spin" width="20" height="20" viewBox="0 0 24 24" fill="none" stroke="currentColor" strokeWidth="2" strokeLinecap="round" strokeLinejoin="round"><path d="M21 12a9 9 0 1 1-6.219-8.56" /></svg>
                감사 중 ({auditProgress.total}개 파일)...
              </>
            ) : (
              <>
                <svg width="24" height="24" viewBox="0 0 24 24" fill="none" stroke="currentColor" strokeWidth="2" strokeLinecap="round" strokeLinejoin="round" style={{ marginRight: '10px' }}><path d="M22 19a2 2 0 0 1-2 2H4a2 2 0 0 1-2-2V5a2 2 0 0 1 2-2h5l2 3h9a2 2 0 0 1 2 2z" /></svg>
                프로젝트 폴더 선택하기
              </>
            )}
          </button>
          <button
            className="btn-primary"
            onClick={() => setShowChecklistModal(true)}
            style={{
              background: 'transparent',
              border: '1px solid var(--border-color)',
              marginLeft: '10px'
            }}
          >
            <svg width="20" height="20" viewBox="0 0 24 24" fill="none" stroke="currentColor" strokeWidth="2" strokeLinecap="round" strokeLinejoin="round" style={{ marginRight: '8px' }}><path d="M9 11l3 3L22 4" /><path d="M21 12v7a2 2 0 0 1-2 2H5a2 2 0 0 1-2-2V5a2 2 0 0 1 2-2h11" /></svg>
            체크리스트 관리
          </button>
        </div>
      </header>

      {showChecklistModal && (
        <ChecklistModal onClose={() => setShowChecklistModal(false)} />
      )}

      {isAuditing && (
        <div style={{ marginBottom: '2rem' }}>
          <div className="flex-between" style={{ marginBottom: '0.5rem', fontSize: '0.9rem' }}>
            <span>전체 {auditProgress.total}개 중 {auditProgress.current}번째 파일 검사 중...</span>
            <span>{Math.round((auditProgress.current / auditProgress.total) * 100)}%</span>
          </div>
          <div style={{ width: '100%', height: '8px', background: 'rgba(255,255,255,0.1)', borderRadius: '4px', overflow: 'hidden' }}>
            <div style={{
              width: `${(auditProgress.current / auditProgress.total) * 100}%`,
              height: '100%',
              background: 'linear-gradient(90deg, #4f46e5, #ec4899)',
              transition: 'width 0.3s ease'
            }} />
          </div>
        </div>
      )}

      {audits.length === 0 && !isAuditing ? (
        <div className="glass-card" style={{ padding: '5rem', textAlign: 'center' }}>
          <div style={{ marginBottom: '1.5rem', opacity: 0.5 }}>
            <svg width="64" height="64" viewBox="0 0 24 24" fill="none" stroke="currentColor" strokeWidth="1" strokeLinecap="round" strokeLinejoin="round"><path d="M14.5 2H6a2 2 0 0 0-2 2v16a2 2 0 0 0 2 2h12a2 2 0 0 0 2-2V7.5L14.5 2z" /><polyline points="14.5 2 14.5 8 20 8" /></svg>
          </div>
          <h2 style={{ marginBottom: '0.5rem' }}>감사할 파일이 없습니다.</h2>
          <p style={{ color: 'var(--text-muted)' }}>상단의 버튼을 클릭하여 소스 코드 파일을 업로드하고 감사를 시작하세요.</p>
        </div>
      ) : (
        <>
          <section className="stats-grid">
            <div className="glass-card stat-item">
              <div className="stat-value" style={{ color: 'var(--text-main)' }}>{stats.total}</div>
              <div className="stat-label">전체 취약점</div>
            </div>
            <div className="glass-card stat-item">
              <div className="stat-value" style={{ color: 'var(--severity-critical)' }}>{stats.critical}</div>
              <div className="stat-label">Critical</div>
            </div>
            <div className="glass-card stat-item">
              <div className="stat-value" style={{ color: 'var(--severity-high)' }}>{stats.high}</div>
              <div className="stat-label">High</div>
            </div>
            <div className="glass-card stat-item">
              <div className="stat-value" style={{ color: 'var(--severity-medium)' }}>{stats.medium}</div>
              <div className="stat-label">Medium</div>
            </div>
          </section>

          <main className="grid-dashboard">
            <aside className="sidebar">
              <h3 style={{ marginBottom: '1rem', paddingLeft: '0.5rem' }}>검사된 파일</h3>
              {audits.map((file, idx) => (
                <div
                  key={idx}
                  className={`glass-card file-nav-item ${selectedFile?.file_path === file.file_path ? 'active' : ''}`}
                  onClick={() => setSelectedFile(file)}
                >
                  <div style={{ fontWeight: 600, fontSize: '0.9rem', marginBottom: '4px' }}>
                    {file.file_path.split(/[/\\]/).pop()}
                  </div>
                  <div style={{ fontSize: '0.75rem', color: 'var(--text-muted)' }}>{file.issues.length}개의 취약점 발견</div>
                </div>
              ))}
            </aside>

            <section className="issue-list">
              {selectedFile ? (
                <>
                  <div className="file-header">
                    <svg width="20" height="20" viewBox="0 0 24 24" fill="none" stroke="currentColor" strokeWidth="2" strokeLinecap="round" strokeLinejoin="round"><path d="M13 2H6a2 2 0 0 0-2 2v16a2 2 0 0 0 2 2h12a2 2 0 0 0 2-2V9z" /><polyline points="13 2 13 9 20 9" /></svg>
                    {selectedFile.file_path}
                  </div>

                  {selectedFile.issues.length === 0 ? (
                    <div className="glass-card" style={{ padding: '2rem', textAlign: 'center', color: 'var(--severity-low)' }}>
                      취약점이 발견되지 않았습니다. 안전한 코드입니다.
                    </div>
                  ) : (
                    selectedFile.issues.map((issue, idx) => (
                      <IssueCard key={idx} issue={issue} />
                    ))
                  )}
                </>
              ) : (
                <div style={{ textAlign: 'center', padding: '5rem', color: 'var(--text-muted)' }}>
                  상세 정보를 보려면 파일을 선택하세요.
                </div>
              )}
            </section>
          </main>
        </>
      )}
    </div>
  );
}

function IssueCard({ issue }: { issue: Issue }) {
  return (
    <div className="glass-card issue-card">
      <div className="issue-header">
        <div>
          <div className="issue-title">{issue.vulnerability}</div>
          <div className="issue-desc">{issue.description}</div>
        </div>
        <span className={`badge severity-${issue.severity.toLowerCase()}`}>
          {issue.severity}
        </span>
      </div>

      <div style={{ marginTop: '1.5rem' }}>
        <div className="flex-between" style={{ marginBottom: '0.5rem' }}>
          <span style={{ fontSize: '0.875rem', fontWeight: 600 }}>코드 분석 및 수정 제안</span>
          <span style={{ fontSize: '0.75rem', color: 'var(--text-muted)' }}>Line {issue.line} | Action: {issue.action_type}</span>
        </div>

        <div className="code-diff-container">
          <div className="diff-original">
            <span className="code-title">Original Code</span>
            <pre><code>{issue.original_code}</code></pre>
          </div>
          <div className="diff-suggested">
            <span className="code-title">Suggested Code</span>
            <pre><code>{issue.suggested_code}</code></pre>
          </div>
        </div>
      </div>
    </div>
  );
}

function ChecklistModal({ onClose }: { onClose: () => void }) {
  const [items, setItems] = useState<string[]>([]);
  const [newItem, setNewItem] = useState('');
  const [isLoading, setIsLoading] = useState(true);
  const [isSaving, setIsSaving] = useState(false);

  useEffect(() => {
    fetchChecklist();
  }, []);

  const fetchChecklist = async () => {
    try {
      const response = await fetch('http://localhost:8001/api/checklist');
      if (response.ok) {
        const data = await response.json();
        setItems(data.items);
      }
    } catch (error) {
      console.error('Error fetching checklist:', error);
    } finally {
      setIsLoading(false);
    }
  };

  const handleAddItem = () => {
    if (newItem.trim()) {
      setItems([...items, newItem.trim()]);
      setNewItem('');
    }
  };

  const handleRemoveItem = (index: number) => {
    setItems(items.filter((_, i) => i !== index));
  };

  const handleSave = async () => {
    setIsSaving(true);
    try {
      const response = await fetch('http://localhost:8001/api/checklist', {
        method: 'POST',
        headers: { 'Content-Type': 'application/json' },
        body: JSON.stringify({ items }),
      });
      if (response.ok) {
        alert('체크리스트가 성공적으로 저장되었습니다.');
        onClose();
      }
    } catch (error) {
      console.error('Error saving checklist:', error);
      alert('저장 중 오류가 발생했습니다.');
    } finally {
      setIsSaving(false);
    }
  };

  return (
    <div className="modal-overlay" onClick={onClose}>
      <div className="glass-card modal-content" onClick={e => e.stopPropagation()} style={{ padding: '2rem' }}>
        <div className="flex-between" style={{ marginBottom: '1.5rem' }}>
          <h2 style={{ fontSize: '1.5rem' }}>보안성 체크리스트 관리</h2>
          <button onClick={onClose} style={{ background: 'transparent', border: 'none', cursor: 'pointer', color: 'var(--text-muted)' }}>
            <svg width="24" height="24" viewBox="0 0 24 24" fill="none" stroke="currentColor" strokeWidth="2" strokeLinecap="round" strokeLinejoin="round"><line x1="18" y1="6" x2="6" y2="18" /><line x1="6" y1="6" x2="18" y2="18" /></svg>
          </button>
        </div>

        <p style={{ color: 'var(--text-muted)', fontSize: '0.9rem', marginBottom: '1.5rem' }}>
          AI가 코드를 검사할 때 참고할 보안 규칙들을 관리합니다. 각 항목은 한 줄씩 적용됩니다.
        </p>

        <div className="checklist-input-group">
          <input
            className="checklist-input"
            placeholder="새로운 보안 규칙 입력..."
            value={newItem}
            onChange={e => setNewItem(e.target.value)}
            onKeyPress={e => e.key === 'Enter' && handleAddItem()}
          />
          <button className="btn-primary" onClick={handleAddItem} style={{ padding: '8px 16px' }}>추가</button>
        </div>

        {isLoading ? (
          <div style={{ textAlign: 'center', padding: '2rem' }}>로딩 중...</div>
        ) : (
          <div className="checklist-items">
            {items.map((item, idx) => (
              <div key={idx} className="checklist-item">
                <span style={{ fontSize: '0.9rem' }}>{item}</span>
                <button className="btn-icon-danger" onClick={() => handleRemoveItem(idx)}>
                  <svg width="18" height="18" viewBox="0 0 24 24" fill="none" stroke="currentColor" strokeWidth="2" strokeLinecap="round" strokeLinejoin="round"><polyline points="3 6 5 6 21 6" /><path d="M19 6v14a2 2 0 0 1-2 2H7a2 2 0 0 1-2-2V6m3 0V4a2 2 0 0 1 2-2h4a2 2 0 0 1 2 2v2" /><line x1="10" y1="11" x2="10" y2="17" /><line x1="14" y1="11" x2="14" y2="17" /></svg>
                </button>
              </div>
            ))}
          </div>
        )}

        <div className="flex-between" style={{ marginTop: 'auto', paddingTop: '1.5rem', borderTop: '1px solid rgba(255,255,255,0.1)' }}>
          <button className="btn-primary" onClick={onClose} style={{ background: 'rgba(255,255,255,0.1)', color: 'white' }}>취소</button>
          <button className="btn-primary" onClick={handleSave} disabled={isSaving}>
            {isSaving ? '저장 중...' : '변경사항 저장'}
          </button>
        </div>
      </div>
    </div>
  );
}

export default App
