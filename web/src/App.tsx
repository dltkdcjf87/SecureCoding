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
  const [selectedPaths, setSelectedPaths] = useState<Set<string>>(new Set());
  const [showChecklistModal, setShowChecklistModal] = useState(false);
  const [isApplyingFix, setIsApplyingFix] = useState(false); // Added state for applying fixes
  const [isCopyCreated, setIsCopyCreated] = useState(false);
  const [fixedPath, setFixedPath] = useState<string | null>(null);
  const [diffModalData, setDiffModalData] = useState<{ issue: Issue, filePath: string } | null>(null);
  const [severityFilter, setSeverityFilter] = useState<string | null>(null);
  const [availableChecklists, setAvailableChecklists] = useState<string[]>([]);
  const [selectedChecklists, setSelectedChecklists] = useState<string[]>([]);
  const folderInputRef = useRef<HTMLInputElement>(null);

  useEffect(() => {
    fetchChecklists();
  }, []);

  const fetchChecklists = async () => {
    try {
      const response = await fetch('http://localhost:8005/api/checklists');
      if (response.ok) {
        const data = await response.json();
        setAvailableChecklists(data);
        // Default selection: all if nothing selected yet or just Default.txt
        if (selectedChecklists.length === 0 && data.includes('Default.txt')) {
          setSelectedChecklists(['Default.txt']);
        }
      }
    } catch (error) {
      console.error('Error fetching checklists:', error);
    }
  };

  const stats = {
    // 필터링된 이슈 리스트 (APPLIED/INFO 제외)
    get filteredIssues() {
      return audits.flatMap(f => f.issues.filter(i =>
        i.action_type !== 'APPLIED' && i.action_type !== 'INFO' &&
        (!severityFilter || i.severity === severityFilter)
      ));
    },
    // 필터링된 파일 리스트
    get filteredFiles() {
      return audits.filter(file => {
        const remainingIssues = file.issues.filter(i => i.action_type !== 'APPLIED' && i.action_type !== 'INFO');
        if (remainingIssues.length === 0) return false;
        if (severityFilter && !remainingIssues.some(i => i.severity === severityFilter)) return false;
        return true;
      });
    },
    totalIssues: 0, // Getter에서 계산됨
    vulnerableFiles: 0,
    critical: audits.reduce((acc, curr) => acc + curr.issues.filter(i => i.severity === 'Critical' && i.action_type !== 'APPLIED' && i.action_type !== 'INFO').length, 0),
    high: audits.reduce((acc, curr) => acc + curr.issues.filter(i => i.severity === 'High' && i.action_type !== 'APPLIED' && i.action_type !== 'INFO').length, 0),
    medium: audits.reduce((acc, curr) => acc + curr.issues.filter(i => i.severity === 'Medium' && i.action_type !== 'APPLIED' && i.action_type !== 'INFO').length, 0),
  };

  // Getter가 지원되지 않는 환경을 고려하여 명시적 계산 (또는 객체 구조 유지)
  const displayStats = {
    totalIssues: stats.filteredIssues.length,
    vulnerableFiles: stats.filteredFiles.length,
    critical: stats.critical,
    high: stats.high,
    medium: stats.medium
  };

  useEffect(() => {
    // 1. Sync Selected File (Find next if current is hidden)
    if (selectedFile) {
      const currentAudit = audits.find(a => a.file_path === selectedFile.file_path);
      if (currentAudit) {
        const remaining = currentAudit.issues.filter(i => i.action_type !== 'APPLIED' && i.action_type !== 'INFO');
        if (remaining.length === 0) {
          const nextAudits = audits.filter(file =>
            file.issues.some(i => i.action_type !== 'APPLIED' && i.action_type !== 'INFO') &&
            (!severityFilter || file.issues.some(i => i.severity === severityFilter))
          );
          if (nextAudits.length > 0) {
            setSelectedFile(nextAudits[0]);
          } else {
            setSelectedFile(null);
          }
        }
      }
    }

    // 2. Sync Selection Count (Remove hidden files from selectedPaths)
    setSelectedPaths(prev => {
      const newSet = new Set(prev);
      let changed = false;

      for (const path of Array.from(newSet)) {
        const audit = audits.find(a => a.file_path === path);
        const hasRemaining = audit?.issues.some(i => i.action_type !== 'APPLIED' && i.action_type !== 'INFO');

        if (!hasRemaining) {
          newSet.delete(path);
          changed = true;
        }
      }

      return changed ? newSet : prev;
    });
  }, [audits, severityFilter]);

  const handleFolderSelect = async (e: React.ChangeEvent<HTMLInputElement>) => {
    const files = e.target.files;
    if (!files || files.length === 0) return;

    // Filter supported files
    const supportedExtensions = ['.py', '.js', '.ts', '.tsx', '.java', '.jsp', '.html', '.css', '.c', '.cpp', '.cc', '.cxx', '.h', '.hpp', '.hxx', '.php'];

    console.log(`[File Selection] Total files found in folder: ${files.length}`);

    const validFiles = Array.from(files).filter(file => {
      const fileName = file.name.toLowerCase();
      const relativePath = file.webkitRelativePath || '';

      const isSupported = supportedExtensions.some(ext => fileName.endsWith(ext));
      const normalizedPath = relativePath.toLowerCase();
      const isIgnored =
        normalizedPath.includes('node_modules/') ||
        normalizedPath.includes('/.venv/') ||
        normalizedPath.includes('/.git/') ||
        normalizedPath.includes('/dist/') ||
        normalizedPath.includes('/build/');

      if (!isSupported) {
        console.debug(`[Filtered] Unsupported extension: ${fileName}`);
      } else if (isIgnored) {
        console.debug(`[Filtered] Ignored path: ${relativePath}`);
      } else {
        console.debug(`[Accepted] ${relativePath}`);
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
    setAudits([]);
    setSelectedFile(null);

    // 알림 추가: 전체 탐색 파일 중 지원되는 파일 개수 안내
    console.log(`[File Selection] Total: ${files.length}, Valid: ${validFiles.length}`);

    const results: FileAudit[] = [];
    const batchSize = 3; // Reduced for safety with Free Tier limits
    for (let i = 0; i < validFiles.length; i += batchSize) {
      const batch = validFiles.slice(i, i + batchSize);

      const formData = new FormData();
      batch.forEach((file, batchIdx) => {
        formData.append('files', file, file.webkitRelativePath || file.name);
        // 진행률 시각적 업데이트 최적화
        setTimeout(() => {
          setAuditProgress(prev => ({
            ...prev,
            current: Math.min(i + batchIdx + 1, validFiles.length)
          }));
        }, batchIdx * 300);
      });

      try {
        const checklistQuery = selectedChecklists.map(c => `checklists=${encodeURIComponent(c)}`).join('&');
        const url = `http://localhost:8005/api/audit-batch?${checklistQuery}`;

        const response = await fetch(url, {
          method: 'POST',
          body: formData,
        });

        if (response.ok) {
          const batchResults: FileAudit[] = await response.json();
          results.push(...batchResults);
          setAudits(prev => [...prev, ...batchResults]);

          if (results.length === batchResults.length) setSelectedFile(batchResults[0]);

          // Update progress upon batch completion
          setAuditProgress(prev => ({
            ...prev,
            current: Math.min(i + batch.length, validFiles.length)
          }));
        }

        // Increased delay to 10s to respect free tier RPM/TPM more conservatively
        if (i + batchSize < validFiles.length) {
          await new Promise(resolve => setTimeout(resolve, 10000));
        }
      } catch (error) {
        console.error(`Error auditing batch starting at ${i}:`, error);
      }
    }

    setIsAuditing(false);
    if (folderInputRef.current) folderInputRef.current.value = '';
    setSelectedPaths(new Set());
    setIsCopyCreated(false);
    setFixedPath(null);
  };

  const togglePathSelection = (path: string) => {
    const newSelection = new Set(selectedPaths);
    if (newSelection.has(path)) {
      newSelection.delete(path);
    } else {
      newSelection.add(path);
    }
    setSelectedPaths(newSelection);
  };

  const ensureProjectCopy = async (basePath: string) => {
    if (isCopyCreated) return true;
    try {
      const response = await fetch('http://localhost:8005/api/create-fix-copy', {
        method: 'POST',
        headers: { 'Content-Type': 'application/json' },
        body: JSON.stringify({ base_path: basePath })
      });
      const data = await response.json();
      if (response.ok || response.status === 200) {
        setIsCopyCreated(true);
        setFixedPath(data.path);
        return true;
      }
      return false;
    } catch (error) {
      console.error('Error creating project copy:', error);
      return false;
    }
  };

  const handleApplyFix = async (filePath: string, issue: Issue) => {
    if (!confirm('원본 보호를 위해 프로젝트 복사본을 생성하고 수정을 적용하시겠습니까?')) return;

    setIsApplyingFix(true);
    const copyOk = await ensureProjectCopy(filePath);
    if (!copyOk) {
      alert('프로젝트 복사본 생성 중 오류가 발생했습니다.');
      setIsApplyingFix(false);
      return;
    }

    try {
      const response = await fetch('http://localhost:8005/api/apply-fix', {
        method: 'POST',
        headers: { 'Content-Type': 'application/json' },
        body: JSON.stringify({
          file_path: filePath,
          original_code: issue.original_code,
          suggested_code: issue.suggested_code
        })
      });

      if (response.ok) {
        alert('성공적으로 수정사항이 적용되었습니다.');
        // Update local state to mark as fixed
        setAudits(prev => prev.map(a => {
          if (a.file_path === filePath) {
            return {
              ...a,
              issues: a.issues.map(i =>
                (i.line === issue.line && i.original_code === issue.original_code)
                  ? { ...i, action_type: 'APPLIED' }
                  : i
              )
            };
          }
          return a;
        }));
      } else {
        const errorData = await response.json();
        alert(`수정 적용 실패: ${errorData.detail || '알 수 없는 오류'}`);
      }
    } catch (error) {
      console.error('Error applying fix:', error);
      alert('수정 요청 중 오류가 발생했습니다.');
    } finally {
      setIsApplyingFix(false);
    }
  };

  const handleApplySelectedFixes = async () => {
    if (selectedPaths.size === 0) {
      alert('적용할 파일을 선택해 주세요.');
      return;
    }

    const targetAudits = audits.filter(a => selectedPaths.has(a.file_path));
    const fixesToApply: any[] = [];
    targetAudits.forEach(audit => {
      audit.issues.forEach(issue => {
        if (issue.action_type !== 'APPLIED' && issue.action_type !== 'INFO' && issue.suggested_code) {
          fixesToApply.push({
            file_path: audit.file_path,
            original_code: issue.original_code,
            suggested_code: issue.suggested_code
          });
        }
      });
    });

    if (fixesToApply.length === 0) {
      alert('선택된 파일들 중 적용 가능한 수정사항이 없습니다.');
      return;
    }

    if (!confirm(`${selectedPaths.size}개의 파일에 대해 프로젝트 복사본을 생성하고 수정을 일괄 적용하시겠습니까?`)) return;

    setIsApplyingFix(true);
    const copyOk = await ensureProjectCopy(targetAudits[0].file_path);
    if (!copyOk) {
      alert('프로젝트 복사본 생성 중 오류가 발생했습니다.');
      setIsApplyingFix(false);
      return;
    }

    try {
      const response = await fetch('http://localhost:8005/api/apply-fix-batch', {
        method: 'POST',
        headers: { 'Content-Type': 'application/json' },
        body: JSON.stringify({ fixes: fixesToApply })
      });

      if (response.ok) {
        alert('선택한 파일들의 수정사항 적용이 완료되었습니다 (복사본 생성됨).');
        setAudits(prev => prev.map(a => {
          if (selectedPaths.has(a.file_path)) {
            return {
              ...a,
              issues: a.issues.map(i =>
                (i.action_type !== 'INFO' && i.suggested_code) ? { ...i, action_type: 'APPLIED' } : i
              )
            };
          }
          return a;
        }));
      } else {
        alert('일부 수정사항 적용 중 오류가 발생했습니다.');
      }
    } catch (error) {
      console.error('Error applying selected fixes:', error);
    } finally {
      setIsApplyingFix(false);
    }
  };

  const handleApplyAllFixes = async () => {
    if (!audits.some(a => a.issues.some(i => i.action_type !== 'APPLIED' && i.action_type !== 'INFO'))) {
      alert('적용할 수정사항이 없습니다.');
      return;
    }

    if (!confirm('전체 파일에 대해 프로젝트 복사본을 생성하고 AI 제안 코드를 일괄 적용하시겠습니까?')) return;

    setIsApplyingFix(true);
    const copyOk = await ensureProjectCopy(audits[0].file_path);
    if (!copyOk) {
      alert('프로젝트 복사본 생성 중 오류가 발생했습니다.');
      setIsApplyingFix(false);
      return;
    }

    try {
      const allFixes: any[] = [];
      audits.forEach(audit => {
        audit.issues.forEach(issue => {
          if (issue.action_type !== 'APPLIED' && issue.action_type !== 'INFO' && issue.suggested_code) {
            allFixes.push({
              file_path: audit.file_path,
              original_code: issue.original_code,
              suggested_code: issue.suggested_code
            });
          }
        });
      });

      if (allFixes.length === 0) return;

      const response = await fetch('http://localhost:8005/api/apply-fix-batch', {
        method: 'POST',
        headers: { 'Content-Type': 'application/json' },
        body: JSON.stringify({ fixes: allFixes })
      });

      if (response.ok) {
        await response.json();
        alert('모든 수정사항 일괄 적용이 완료되었습니다.');
        // Update all state to APPLIED
        setAudits(prev => prev.map(a => ({
          ...a,
          issues: a.issues.map(i =>
            (i.action_type !== 'INFO' && i.suggested_code) ? { ...i, action_type: 'APPLIED' } : i
          )
        })));
      } else {
        alert('일괄 적용 중 일부 오류가 발생했습니다.');
      }
    } catch (error) {
      console.error('Error applying batch fixes:', error);
      alert('일괄 적용 요청 중 오류가 발생했습니다.');
    } finally {
      setIsApplyingFix(false);
    }
  };

  return (
    <div className="container">
      <header className="flex-between" style={{ marginBottom: '3rem' }}>
        <div>
          <h1 style={{ fontSize: '2.5rem', fontWeight: 800, letterSpacing: '-0.02em', marginBottom: '0.5rem' }}>
            Secure<span style={{ color: 'var(--accent-color)' }}>Audit</span>
          </h1>
          <p style={{ color: 'var(--text-muted)', fontSize: '1.1rem' }}>AI 기반 시큐어 코딩 및 취약점 진단 솔루션</p>
        </div>
        <div style={{ display: 'flex', gap: '10px' }}>
          <button className="btn-primary" onClick={() => folderInputRef.current?.click()} disabled={isAuditing}>
            <svg width="20" height="20" viewBox="0 0 24 24" fill="none" stroke="currentColor" strokeWidth="2" strokeLinecap="round" strokeLinejoin="round" style={{ marginRight: '8px' }}><path d="M22 19a2 2 0 0 1-2 2H4a2 2 0 0 1-2-2V5a2 2 0 0 1 2-2h5l2 3h9a2 2 0 0 1 2 2z" /></svg>
            프로젝트 폴더 선택
          </button>
          <input
            type="file"
            ref={folderInputRef}
            onChange={handleFolderSelect}
            style={{ display: 'none' }}
            {...({ webkitdirectory: "", directory: "" } as any)}
          />
          <button
            className="btn-primary"
            onClick={() => setShowChecklistModal(true)}
            style={{
              background: 'transparent',
              border: '1px solid var(--border-color)',
              position: 'relative'
            }}
          >
            <svg width="20" height="20" viewBox="0 0 24 24" fill="none" stroke="currentColor" strokeWidth="2" strokeLinecap="round" strokeLinejoin="round" style={{ marginRight: '8px' }}><path d="M9 11l3 3L22 4" /><path d="M21 12v7a2 2 0 0 1-2 2H5a2 2 0 0 1-2-2V5a2 2 0 0 1 2-2h11" /></svg>
            체크리스트 관리
            {selectedChecklists.length > 0 && (
              <span style={{
                position: 'absolute',
                top: '-8px',
                right: '-8px',
                background: 'var(--accent-color)',
                color: 'white',
                fontSize: '0.7rem',
                padding: '2px 6px',
                borderRadius: '10px',
                fontWeight: 800
              }}>
                {selectedChecklists.length}
              </span>
            )}
          </button>
        </div>
      </header>

      {isCopyCreated && fixedPath && (
        <div className="glass-card" style={{
          marginBottom: '2rem',
          padding: '1rem 1.5rem',
          borderLeft: '4px solid #10b981',
          background: 'rgba(16, 185, 129, 0.05)',
          display: 'flex',
          alignItems: 'center',
          gap: '12px'
        }}>
          <svg width="20" height="20" viewBox="0 0 24 24" fill="none" stroke="#10b981" strokeWidth="2" strokeLinecap="round" strokeLinejoin="round"><path d="M22 11.08V12a10 10 0 1 1-5.93-9.14" /><polyline points="22 4 12 14.01 9 11.01" /></svg>
          <div>
            <span style={{ fontWeight: 600, color: '#34d399', marginRight: '8px' }}>수정본 생성됨:</span>
            <code style={{ fontSize: '0.9rem', color: 'var(--text-main)', opacity: 0.9 }}>{fixedPath}</code>
          </div>
        </div>
      )}

      {showChecklistModal && (
        <ChecklistModal
          onClose={() => setShowChecklistModal(false)}
          availableChecklists={availableChecklists}
          selectedChecklists={selectedChecklists}
          setSelectedChecklists={setSelectedChecklists}
          fetchChecklists={fetchChecklists}
        />
      )}

      {diffModalData && (
        <DiffModal
          data={diffModalData}
          onClose={() => setDiffModalData(null)}
        />
      )}

      {isAuditing && (
        <div className="progress-overlay">
          <div className="progress-content pulse-animation">
            <div style={{ position: 'relative', width: '80px', height: '80px', margin: '0 auto 1.5rem' }}>
              <svg className="animate-spin" width="80" height="80" viewBox="0 0 24 24" fill="none" stroke="var(--primary)" strokeWidth="1.5" strokeLinecap="round" strokeLinejoin="round" style={{ opacity: 0.8 }}><path d="M21 12a9 9 0 1 1-6.219-8.56" /></svg>
              <div style={{ position: 'absolute', inset: 0, display: 'flex', alignItems: 'center', justifyContent: 'center' }}>
                <svg width="32" height="32" viewBox="0 0 24 24" fill="none" stroke="var(--primary)" strokeWidth="2" strokeLinecap="round" strokeLinejoin="round"><path d="M12 22s8-4 8-10V5l-8-3-8 3v7c0 6 8 10 8 10z" /></svg>
              </div>
            </div>
            <div className="scanning-text">보안 취약점 감사 분석 중...</div>
            <p style={{ color: 'var(--text-muted)', fontSize: '1.2rem', marginBottom: '1rem' }}>
              <span style={{ color: 'white', fontWeight: 700 }}>{auditProgress.current}</span> / {auditProgress.total} 개 파일 분석 완료
            </p>
            <div className="progress-bar-container">
              <div
                className="progress-bar-fill"
                style={{ width: `${(auditProgress.current / auditProgress.total) * 100}%` }}
              ></div>
            </div>
            <div style={{ marginTop: '1rem', fontSize: '0.9rem', color: 'var(--text-muted)', opacity: 0.7 }}>
              AI가 각 파일의 로직을 정밀 분석하고 있습니다. 잠시만 기다려 주세요.
            </div>
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
              <div className="stat-value" style={{ color: 'var(--text-main)' }}>{displayStats.totalIssues}</div>
              <div className="stat-label">{severityFilter ? `${severityFilter} 이슈` : '전체 취약점'}</div>
            </div>
            <div className="glass-card stat-item">
              <div className="stat-value" style={{ color: 'var(--text-main)' }}>{displayStats.vulnerableFiles}</div>
              <div className="stat-label">대상 파일 수</div>
            </div>
            <div className={`glass-card stat-item ${severityFilter === 'Critical' ? 'active-filter' : ''}`} style={{ borderLeft: '4px solid var(--severity-critical)', cursor: 'pointer' }} onClick={() => setSeverityFilter(prev => prev === 'Critical' ? null : 'Critical')}>
              <div className="stat-value" style={{ color: 'var(--severity-critical)' }}>{displayStats.critical}</div>
              <div className="stat-label">Critical</div>
            </div>
            <div className={`glass-card stat-item ${severityFilter === 'High' ? 'active-filter' : ''}`} style={{ borderLeft: '4px solid var(--severity-high)', cursor: 'pointer' }} onClick={() => setSeverityFilter(prev => prev === 'High' ? null : 'High')}>
              <div className="stat-value" style={{ color: 'var(--severity-high)' }}>{displayStats.high}</div>
              <div className="stat-label">High</div>
            </div>
            <div className={`glass-card stat-item ${severityFilter === 'Medium' ? 'active-filter' : ''}`} style={{ borderLeft: '4px solid var(--severity-medium)', cursor: 'pointer' }} onClick={() => setSeverityFilter(prev => prev === 'Medium' ? null : 'Medium')}>
              <div className="stat-value" style={{ color: 'var(--severity-medium)' }}>{displayStats.medium}</div>
              <div className="stat-label">Medium</div>
            </div>
            <div className="glass-card stat-item" style={{ borderLeft: '2px solid #10b981', cursor: 'pointer', transition: 'all 0.2s' }} onClick={handleApplySelectedFixes}>
              <div className="stat-value" style={{ color: '#34d399', display: 'flex', alignItems: 'center', justifyContent: 'center', gap: '8px' }}>
                <svg width="24" height="24" viewBox="0 0 24 24" fill="none" stroke="currentColor" strokeWidth="2" strokeLinecap="round" strokeLinejoin="round"><path d="M12 22c5.523 0 10-4.477 10-10S17.523 2 12 2 2 6.477 2 12s4.477 10 10 10z" /><path d="m9 12 2 2 4-4" /></svg>
                {selectedPaths.size} Files
              </div>
              <div className="stat-label" style={{ fontWeight: 700 }}>선택 항목 적용</div>
            </div>
            <div className="glass-card stat-item" style={{ borderLeft: '2px solid #4f46e5', cursor: 'pointer', transition: 'all 0.2s' }} onClick={handleApplyAllFixes}>
              <div className="stat-value" style={{ color: '#818cf8', display: 'flex', alignItems: 'center', justifyContent: 'center', gap: '8px' }}>
                <svg width="24" height="24" viewBox="0 0 24 24" fill="none" stroke="currentColor" strokeWidth="2" strokeLinecap="round" strokeLinejoin="round"><path d="M12 22c5.523 0 10-4.477 10-10S17.523 2 12 2 2 6.477 2 12s4.477 10 10 10z" /><path d="m9 12 2 2 4-4" /></svg>
                {isApplyingFix ? '적용 중...' : 'Auto-Fix'}
              </div>
              <div className="stat-label" style={{ fontWeight: 700 }}>AI 가이드 일괄 적용</div>
            </div>
          </section>

          <main className="grid-dashboard">
            <aside className="sidebar">
              <div className="flex-between" style={{ marginBottom: '1rem', padding: '0 0.5rem' }}>
                <h3 style={{ margin: 0 }}>검사된 파일 ({displayStats.vulnerableFiles})</h3>
                {severityFilter && (
                  <span
                    onClick={() => setSeverityFilter(null)}
                    style={{ fontSize: '0.7rem', color: 'var(--accent-color)', cursor: 'pointer', fontWeight: 700 }}
                  >
                    필터 해제 ✕
                  </span>
                )}
              </div>

              {/* Select All Checkbox */}
              {audits.length > 0 && (
                <div style={{
                  padding: '0 0.5rem 0.8rem',
                  display: 'flex',
                  alignItems: 'center',
                  gap: '8px',
                  borderBottom: '1px solid rgba(255,255,255,0.05)',
                  marginBottom: '1rem'
                }}>
                  <input
                    type="checkbox"
                    id="select-all-checkbox"
                    style={{ width: '18px', height: '18px', cursor: 'pointer' }}
                    checked={
                      audits.filter(file => {
                        const remainingIssues = file.issues.filter(i => i.action_type !== 'APPLIED' && i.action_type !== 'INFO');
                        if (remainingIssues.length === 0) return false;
                        if (severityFilter && !remainingIssues.some(i => i.severity === severityFilter)) return false;
                        return true;
                      }).length > 0 &&
                      audits.filter(file => {
                        const remainingIssues = file.issues.filter(i => i.action_type !== 'APPLIED' && i.action_type !== 'INFO');
                        if (remainingIssues.length === 0) return false;
                        if (severityFilter && !remainingIssues.some(i => i.severity === severityFilter)) return false;
                        return true;
                      }).every(file => selectedPaths.has(file.file_path))
                    }
                    onChange={(e) => {
                      const filteredFiles = audits.filter(file => {
                        const remainingIssues = file.issues.filter(i => i.action_type !== 'APPLIED' && i.action_type !== 'INFO');
                        if (remainingIssues.length === 0) return false;
                        if (severityFilter && !remainingIssues.some(i => i.severity === severityFilter)) return false;
                        return true;
                      });

                      const newSelection = new Set(selectedPaths);
                      if (e.target.checked) {
                        filteredFiles.forEach(f => newSelection.add(f.file_path));
                      } else {
                        filteredFiles.forEach(f => newSelection.delete(f.file_path));
                      }
                      setSelectedPaths(newSelection);
                    }}
                  />
                  <label htmlFor="select-all-checkbox" style={{ fontSize: '0.85rem', cursor: 'pointer', color: 'var(--text-muted)' }}>
                    현재 리스트 전체 선택
                  </label>
                </div>
              )}
              {audits
                .filter(file => {
                  const remainingIssues = file.issues.filter(i => i.action_type !== 'APPLIED' && i.action_type !== 'INFO');
                  if (remainingIssues.length === 0) return false;
                  if (severityFilter && !remainingIssues.some(i => i.severity === severityFilter)) return false;
                  return true;
                })
                .map((file, idx) => {
                  const remainingCount = file.issues.filter(i => i.action_type !== 'APPLIED' && i.action_type !== 'INFO').length;
                  return (
                    <div key={idx} style={{ display: 'flex', alignItems: 'center', gap: '8px', marginBottom: '8px' }}>
                      <input
                        type="checkbox"
                        checked={selectedPaths.has(file.file_path)}
                        onChange={() => togglePathSelection(file.file_path)}
                        style={{ width: '18px', height: '18px', cursor: 'pointer' }}
                      />
                      <div
                        className={`glass-card file-nav-item ${selectedFile?.file_path === file.file_path ? 'active' : ''}`}
                        onClick={() => setSelectedFile(file)}
                        style={{ flex: 1, marginBottom: 0 }}
                      >
                        <div style={{ fontWeight: 600, fontSize: '0.9rem', marginBottom: '4px' }}>
                          {file.file_path.split(/[/\\]/).pop()}
                        </div>
                        <div style={{ fontSize: '0.75rem', color: 'var(--text-muted)' }}>{remainingCount}개의 잔여 취약점</div>
                      </div>
                    </div>
                  );
                })}
            </aside>

            <section className="issue-list">
              {selectedFile ? (
                <>
                  <div className="file-header">
                    <svg width="20" height="20" viewBox="0 0 24 24" fill="none" stroke="currentColor" strokeWidth="2" strokeLinecap="round" strokeLinejoin="round"><path d="M13 2H6a2 2 0 0 0-2 2v16a2 2 0 0 0 2 2h12a2 2 0 0 0 2-2V9z" /><polyline points="13 2 13 9 20 9" /></svg>
                    {selectedFile.file_path}
                  </div>

                  {selectedFile.issues.filter(i => i.action_type !== 'APPLIED' && i.action_type !== 'INFO' && (!severityFilter || i.severity === severityFilter)).length === 0 ? (
                    <div className="glass-card" style={{ padding: '2rem', textAlign: 'center', color: 'var(--severity-low)' }}>
                      해당 조건의 취약점이 발견되지 않았습니다.
                    </div>
                  ) : (
                    selectedFile.issues
                      .filter(i => i.action_type !== 'APPLIED' && i.action_type !== 'INFO' && (!severityFilter || i.severity === severityFilter))
                      .map((issue, idx) => (
                        <IssueCard
                          key={idx}
                          issue={issue}
                          onApplyFix={() => handleApplyFix(selectedFile.file_path, issue)}
                          onShowDiff={() => setDiffModalData({ issue, filePath: selectedFile.file_path })}
                        />
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

function IssueCard({ issue, onApplyFix, onShowDiff }: { issue: Issue, onApplyFix: () => void, onShowDiff: () => void }) {
  return (
    <div className="glass-card issue-card">
      <div className="issue-header">
        <div style={{ flex: 1 }}>
          <div className="issue-title" style={{ display: 'flex', alignItems: 'center', gap: '8px' }}>
            {issue.vulnerability}
            <button
              onClick={onShowDiff}
              style={{ padding: '2px 8px', fontSize: '0.65rem', background: 'rgba(255,255,255,0.1)', border: '1px solid var(--border-color)', borderRadius: '4px', cursor: 'pointer', color: 'var(--text-muted)' }}
            >
              상세 비교 ↗
            </button>
          </div>
          <div className="issue-desc">{issue.description}</div>
        </div>
        <span className={`badge severity-${issue.severity.toLowerCase()}`}>
          {issue.severity}
        </span>
      </div>

      <div style={{ marginTop: '1.2rem' }}>
        <div className="flex-between" style={{ marginBottom: '0.5rem' }}>
          <span style={{ fontSize: '0.875rem', fontWeight: 600 }}>코드 분석 및 수정 제안</span>
          <div style={{ display: 'flex', alignItems: 'center', gap: '12px' }}>
            <span style={{ fontSize: '0.75rem', color: 'var(--text-muted)' }}>Line {issue.line} | Action: {issue.action_type}</span>
            {issue.action_type === 'APPLIED' ? (
              <span className="badge" style={{ background: 'rgba(52, 211, 153, 0.2)', color: '#34d399', border: '1px solid #34d399' }}>적용 완료</span>
            ) : issue.action_type !== 'INFO' && issue.suggested_code && (
              <button
                className="btn-primary"
                onClick={onApplyFix}
                style={{ fontSize: '0.7rem', padding: '4px 12px', background: 'var(--accent-color)' }}
              >
                AI 가이드 적용
              </button>
            )}
          </div>
        </div>

        <div className="code-diff-vertical">
          <div className="diff-item original">
            <div className="diff-label">Old</div>
            <pre><code>{issue.original_code}</code></pre>
          </div>
          <div className="diff-item suggested">
            <div className="diff-label">New</div>
            <pre><code>{issue.suggested_code}</code></pre>
          </div>
        </div>
      </div>
    </div>
  );
}

function DiffModal({ data, onClose }: { data: { issue: Issue, filePath: string }, onClose: () => void }) {
  const { issue, filePath } = data;
  return (
    <div className="modal-overlay" onClick={onClose}>
      <div className="glass-card modal-content" onClick={e => e.stopPropagation()} style={{ padding: '2rem', width: '90vw', height: '90vh', maxWidth: '1400px' }}>
        <div className="flex-between" style={{ marginBottom: '1.5rem' }}>
          <div>
            <div style={{ display: 'flex', alignItems: 'center', gap: '10px', marginBottom: '4px' }}>
              <span className={`badge severity-${issue.severity.toLowerCase()}`}>{issue.severity}</span>
              <h2 style={{ fontSize: '1.3rem' }}>{issue.vulnerability}</h2>
            </div>
            <p style={{ color: 'var(--text-muted)', fontSize: '0.9rem' }}>{filePath} (Line {issue.line})</p>
          </div>
          <button onClick={onClose} style={{ background: 'transparent', border: 'none', cursor: 'pointer', color: 'var(--text-muted)' }}>
            <svg width="24" height="24" viewBox="0 0 24 24" fill="none" stroke="currentColor" strokeWidth="2" strokeLinecap="round" strokeLinejoin="round"><line x1="18" y1="6" x2="6" y2="18" /><line x1="6" y1="6" x2="18" y2="18" /></svg>
          </button>
        </div>

        <div style={{ marginBottom: '1.5rem', padding: '1rem', background: 'rgba(255,255,255,0.03)', borderRadius: '8px', borderLeft: '4px solid var(--accent-color)' }}>
          <p style={{ fontSize: '1rem', lineHeight: 1.5 }}>{issue.description}</p>
        </div>

        <div className="diff-grid-overlay" style={{ display: 'grid', gridTemplateColumns: '1fr 1fr', gap: '1.5rem', height: 'calc(100% - 180px)', overflow: 'hidden' }}>
          <div className="diff-pane" style={{ display: 'flex', flexDirection: 'column' }}>
            <div style={{ padding: '8px 16px', background: 'rgba(239, 68, 68, 0.1)', color: '#f87171', fontSize: '0.8rem', fontWeight: 700, borderRadius: '4px 4px 0 0' }}>ORIGINAL</div>
            <pre style={{ margin: 0, padding: '1rem', background: 'rgba(0,0,0,0.2)', flex: 1, overflow: 'auto', borderRadius: '0 0 8px 8px', fontSize: '0.9rem', border: '1px solid rgba(239, 68, 68, 0.2)' }}>
              <code>{issue.original_code}</code>
            </pre>
          </div>
          <div className="diff-pane" style={{ display: 'flex', flexDirection: 'column' }}>
            <div style={{ padding: '8px 16px', background: 'rgba(16, 185, 129, 0.1)', color: '#34d399', fontSize: '0.8rem', fontWeight: 700, borderRadius: '4px 4px 0 0' }}>SUGGESTED</div>
            <pre style={{ margin: 0, padding: '1rem', background: 'rgba(0,0,0,0.2)', flex: 1, overflow: 'auto', borderRadius: '0 0 8px 8px', fontSize: '0.9rem', border: '1px solid rgba(16, 185, 129, 0.2)' }}>
              <code>{issue.suggested_code}</code>
            </pre>
          </div>
        </div>
      </div>
    </div>
  );
}

function ChecklistModal({
  onClose,
  availableChecklists,
  selectedChecklists,
  setSelectedChecklists,
  fetchChecklists
}: {
  onClose: () => void,
  availableChecklists: string[],
  selectedChecklists: string[],
  setSelectedChecklists: React.Dispatch<React.SetStateAction<string[]>>,
  fetchChecklists: () => Promise<void>
}) {
  const [editingFile, setEditingFile] = useState<string | null>(null);
  const [text, setText] = useState('');
  const [newChecklistName, setNewChecklistName] = useState('');
  const [isLoading, setIsLoading] = useState(false);
  const [isSaving, setIsSaving] = useState(false);

  useEffect(() => {
    if (editingFile) {
      fetchChecklistContent(editingFile);
    }
  }, [editingFile]);

  const fetchChecklistContent = async (filename: string) => {
    setIsLoading(true);
    try {
      const response = await fetch(`http://localhost:8005/api/checklists/${encodeURIComponent(filename)}`);
      if (response.ok) {
        const data = await response.json();
        setText(data.text);
      }
    } catch (error) {
      console.error('Error fetching checklist content:', error);
    } finally {
      setIsLoading(false);
    }
  };

  const handleSave = async () => {
    if (!editingFile) return;
    setIsSaving(true);
    try {
      const response = await fetch('http://localhost:8005/api/checklists', {
        method: 'POST',
        headers: { 'Content-Type': 'application/json' },
        body: JSON.stringify({ name: editingFile, text }),
      });
      if (response.ok) {
        alert('저장되었습니다.');
      }
    } catch (error) {
      console.error('Error saving checklist:', error);
      alert('저장 중 오류가 발생했습니다.');
    } finally {
      setIsSaving(false);
    }
  };

  const handleAddNew = async () => {
    if (!newChecklistName.trim()) return;
    const filename = newChecklistName.endsWith('.txt') ? newChecklistName : `${newChecklistName}.txt`;

    try {
      const response = await fetch('http://localhost:8005/api/checklists', {
        method: 'POST',
        headers: { 'Content-Type': 'application/json' },
        body: JSON.stringify({ name: filename, text: '# 새 체크리스트 가이드' }),
      });
      if (response.ok) {
        setNewChecklistName('');
        await fetchChecklists();
        setEditingFile(filename);
        // Automatically select the new one
        if (!selectedChecklists.includes(filename)) {
          setSelectedChecklists(prev => [...prev, filename]);
        }
      }
    } catch (error) {
      console.error('Error adding checklist:', error);
    }
  };

  const handleDelete = async (filename: string, e: React.MouseEvent) => {
    e.stopPropagation();
    if (!confirm(`'${filename}' 체크리스트를 삭제하시겠습니까?`)) return;

    try {
      const response = await fetch(`http://localhost:8005/api/checklists/${encodeURIComponent(filename)}`, {
        method: 'DELETE',
      });
      if (response.ok) {
        if (editingFile === filename) {
          setEditingFile(null);
          setText('');
        }
        setSelectedChecklists(prev => prev.filter(c => c !== filename));
        await fetchChecklists();
      }
    } catch (error) {
      console.error('Error deleting checklist:', error);
    }
  };

  const toggleSelection = (filename: string) => {
    setSelectedChecklists(prev =>
      prev.includes(filename) ? prev.filter(c => c !== filename) : [...prev, filename]
    );
  };

  return (
    <div className="modal-overlay" onClick={onClose}>
      <div className="glass-card modal-content wide" onClick={e => e.stopPropagation()} style={{ padding: '2rem', display: 'flex', flexDirection: 'column' }}>
        <div className="flex-between" style={{ marginBottom: '1.5rem' }}>
          <div>
            <h2 style={{ fontSize: '1.8rem', fontWeight: 800 }}>체크리스트 통합 관리</h2>
            <p style={{ color: 'var(--text-muted)' }}>감사에 사용할 체크리스트를 선택하거나 내용을 편집하세요.</p>
          </div>
          <button onClick={onClose} className="btn-icon">
            <svg width="24" height="24" viewBox="0 0 24 24" fill="none" stroke="currentColor" strokeWidth="2"><line x1="18" y1="6" x2="6" y2="18" /><line x1="6" y1="6" x2="18" y2="18" /></svg>
          </button>
        </div>

        <div style={{ display: 'grid', gridTemplateColumns: '300px 1fr', gap: '2rem', flex: 1, minHeight: 0 }}>
          {/* Left Side: List */}
          <div style={{ display: 'flex', flexDirection: 'column', gap: '1rem', borderRight: '1px solid var(--border-color)', paddingRight: '1.5rem' }}>
            <div style={{ display: 'flex', gap: '8px' }}>
              <input
                className="checklist-input"
                value={newChecklistName}
                onChange={e => setNewChecklistName(e.target.value)}
                placeholder="새 파일 이름 (예: SQL보안)"
                onKeyDown={e => e.key === 'Enter' && handleAddNew()}
              />
              <button className="btn-primary" style={{ padding: '8px 12px' }} onClick={handleAddNew}>추가</button>
            </div>

            <div style={{ flex: 1, overflowY: 'auto', display: 'flex', flexDirection: 'column', gap: '8px' }}>
              {availableChecklists.map(name => (
                <div
                  key={name}
                  className={`checklist-item ${editingFile === name ? 'active' : ''}`}
                  onClick={() => setEditingFile(name)}
                  style={{
                    cursor: 'pointer',
                    background: editingFile === name ? 'rgba(59, 130, 246, 0.15)' : 'rgba(255,255,255,0.03)',
                    borderColor: editingFile === name ? 'var(--primary)' : 'rgba(255,255,255,0.1)'
                  }}
                >
                  <div style={{ display: 'flex', alignItems: 'center', gap: '12px' }}>
                    <input
                      type="checkbox"
                      checked={selectedChecklists.includes(name)}
                      onChange={() => toggleSelection(name)}
                      onClick={e => e.stopPropagation()}
                      style={{ width: '18px', height: '18px', cursor: 'pointer' }}
                    />
                    <span style={{ fontWeight: editingFile === name ? 700 : 400 }}>{name}</span>
                  </div>
                  <button
                    className="btn-icon-danger"
                    onClick={(e) => handleDelete(name, e)}
                    title="삭제"
                  >
                    <svg width="16" height="16" viewBox="0 0 24 24" fill="none" stroke="currentColor" strokeWidth="2"><path d="M3 6h18" /><path d="M19 6v14a2 2 0 0 1-2 2H7a2 2 0 0 1-2-2V6" /><path d="M8 6V4a2 2 0 0 1 2-2h4a2 2 0 0 1 2 2v2" /></svg>
                  </button>
                </div>
              ))}
            </div>
          </div>

          {/* Right Side: Editor */}
          <div style={{ display: 'flex', flexDirection: 'column' }}>
            {editingFile ? (
              <>
                <div className="flex-between" style={{ marginBottom: '1rem' }}>
                  <h3 style={{ fontSize: '1.2rem', color: 'var(--primary)' }}>{editingFile} 편집 중</h3>
                  <button className="btn-primary" onClick={handleSave} disabled={isSaving}>
                    {isSaving ? '저장 중...' : '변경사항 저장'}
                  </button>
                </div>
                {isLoading ? (
                  <div style={{ flex: 1, display: 'flex', alignItems: 'center', justifyContent: 'center' }}>내용 로딩 중...</div>
                ) : (
                  <textarea
                    className="checklist-editor"
                    style={{ flex: 1, margin: 0 }}
                    value={text}
                    onChange={e => setText(e.target.value)}
                    spellCheck={false}
                  />
                )}
              </>
            ) : (
              <div style={{ flex: 1, display: 'flex', alignItems: 'center', justifyContent: 'center', color: 'var(--text-muted)', fontSize: '1.1rem', background: 'rgba(0,0,0,0.1)', borderRadius: '12px' }}>
                왼쪽 목록에서 편집할 체크리스트 파일을 선택하세요.
              </div>
            )}
          </div>
        </div>

        <div style={{ marginTop: '1.5rem', textAlign: 'right' }}>
          <button className="btn-primary" style={{ background: 'var(--text-muted)' }} onClick={onClose}>닫기</button>
        </div>
      </div>
    </div>
  );
}

export default App
