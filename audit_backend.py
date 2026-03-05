import os
import json
import uvicorn
from fastapi import FastAPI, UploadFile, File, HTTPException
from fastapi.middleware.cors import CORSMiddleware
import google.generativeai as genai
from pydantic import BaseModel
from typing import List

app = FastAPI(title="Security Audit API")

# CORS settings for React frontend
app.add_middleware(
    CORSMiddleware,
    allow_origins=["*"],
    allow_methods=["*"],
    allow_headers=["*"],
)

# Configure Gemini API
GOOGLE_API_KEY = os.getenv("GOOGLE_API_KEY")
if not GOOGLE_API_KEY:
    print("Warning: GOOGLE_API_KEY environment variable is not set.")
else:
    genai.configure(api_key=GOOGLE_API_KEY)

class Issue(BaseModel):
    line: int
    vulnerability: str
    severity: str
    description: str
    original_code: str
    suggested_code: str
    action_type: str

class AuditResult(BaseModel):
    file_path: str
    issues: List[Issue]

class ChecklistUpdate(BaseModel):
    items: List[str]

# Prompt template for Gemini
AUDIT_PROMPT = """
당신은 전문 보안 시큐어 코딩 및 개인정보보호 감사관입니다.
다음 제공된 '체크리스트 가이드'의 규칙을 최우선으로 적용하여, 입력된 소스 코드에서 보안 취약점과 개인정보 노출 위험을 찾아내세요.

[체크리스트 가이드]
{checklist}

[분석할 소스 코드 - 파일명: {filename}]
{code}

반드시 다음 JSON 형식으로 응답하세요. 다른 설명은 포함하지 마세요.
[
  {{
    "line": 124,
    "vulnerability": "SQL Injection",
    "severity": "Critical",
    "description": "MyBatis에서 ${{}}를 사용하여 SQL 인젝션 위험이 있습니다.",
    "original_code": "SELECT * FROM users WHERE id = '${{id}}'",
    "suggested_code": "SELECT * FROM users WHERE id = #{{id}}",
    "action_type": "REPLACE"
  }}
]
"""

async def perform_audit(filename: str, code: str) -> List[Issue]:
    # Read checklist
    checklist_path = r"c:\Users\dltkd\antigravity\보안취약점검사\보안성체크리스트.txt"
    if not os.path.exists(checklist_path):
         return [Issue(line=0, vulnerability="System Error", severity="High", description="체크리스트 파일을 찾을 수 없습니다.", original_code="", suggested_code="", action_type="ERROR")]

    with open(checklist_path, "r", encoding="utf-8") as f:
        checklist = f.read()

    try:
        # Call Gemini
        model = genai.GenerativeModel("gemini-2.0-flash")
        prompt = AUDIT_PROMPT.format(
            checklist=checklist,
            filename=filename,
            code=code
        )
        
        response = model.generate_content(prompt)
        
        # Clean response text
        resp_text = response.text.strip()
        if resp_text.startswith("```json"):
            resp_text = resp_text[7:-3].strip()
        elif resp_text.startswith("```"):
            resp_text = resp_text[3:-3].strip()
            
        issues_data = json.loads(resp_text)
        return [Issue(**issue) for issue in issues_data]
    except Exception as api_err:
        print(f"Gemini API Error for {filename}: {api_err}")
        if "429" in str(api_err):
             return [Issue(
                line=1,
                vulnerability="API Quota Exceeded",
                severity="Medium",
                description="Gemini API 쿼터가 초과되었습니다.",
                original_code=code.split('\n')[0] if code else "source code",
                suggested_code="API 키의 쿼터를 확인하세요.",
                action_type="INFO"
            )]
        raise api_err

@app.post("/api/audit", response_model=AuditResult)
async def audit_single_file(file: UploadFile = File(...)):
    supported_extensions = ('.py', '.js', '.ts', '.tsx', '.java', '.jsp', '.html', '.css', '.c', '.cpp', '.cc', '.cxx', '.h', '.hpp', '.hxx', '.php')
    if not file.filename.lower().endswith(supported_extensions):
        raise HTTPException(status_code=400, detail="Unsupported file extension")
    
    try:
        content = await file.read()
        code = content.decode("utf-8")
        issues = await perform_audit(file.filename, code)
        return {"file_path": file.filename, "issues": issues}
    except Exception as e:
        print(f"Failed to audit {file.filename}: {e}")
        raise HTTPException(status_code=500, detail=str(e))

@app.post("/api/audit-batch", response_model=List[AuditResult])
async def audit_batch(files: List[UploadFile] = File(...)):
    results = []
    supported_extensions = ('.py', '.js', '.ts', '.tsx', '.java', '.jsp', '.html', '.css', '.c', '.cpp', '.cc', '.cxx', '.h', '.hpp', '.hxx', '.php')
    
    for file in files:
        if file.filename.lower().endswith(supported_extensions):
            try:
                content = await file.read()
                code = content.decode("utf-8")
                issues = await perform_audit(file.filename, code)
                # Keep original filename/path provided by frontend
                results.append({"file_path": file.filename, "issues": issues})
            except Exception as e:
                print(f"Failed to audit {file.filename}: {e}")
                continue
    
    return results

@app.get("/api/checklist")
async def get_checklist():
    checklist_path = r"c:\Users\dltkd\antigravity\보안취약점검사\보안성체크리스트.txt"
    if not os.path.exists(checklist_path):
        return {"items": []}
    
    with open(checklist_path, "r", encoding="utf-8") as f:
        # Filter out empty lines or titles if needed, but for now return all meaningful lines
        lines = [line.strip() for line in f.readlines() if line.strip()]
    return {"items": lines}

@app.post("/api/checklist")
async def update_checklist(update: ChecklistUpdate):
    checklist_path = r"c:\Users\dltkd\antigravity\보안취약점검사\보안성체크리스트.txt"
    try:
        with open(checklist_path, "w", encoding="utf-8") as f:
            for item in update.items:
                f.write(item + "\n")
        return {"status": "success"}
    except Exception as e:
        raise HTTPException(status_code=500, detail=str(e))

if __name__ == "__main__":
    uvicorn.run(app, host="0.0.0.0", port=8001)
