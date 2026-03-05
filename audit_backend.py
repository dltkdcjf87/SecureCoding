import os
import json
import uvicorn
import time
from fastapi import FastAPI, UploadFile, File, HTTPException
from fastapi.middleware.cors import CORSMiddleware
import google.generativeai as genai
from pydantic import BaseModel
from typing import List
from dotenv import load_dotenv

# Load environment variables from .env file
load_dotenv()

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
    text: str

# Prompt template for Single File Audit
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

# Prompt template for Batch Audit (Multiple Files)
BATCH_AUDIT_PROMPT = """
당신은 전문 보안 시큐어 코딩 및 개인정보보호 감사관입니다.
제공된 '체크리스트 가이드'를 기반으로 여러 소스 파일들을 한꺼번에 분석하여 보안 취약점을 찾아내세요.

[체크리스트 가이드]
{checklist}

[분석할 소스 파일들]
{files_content}

반드시 다음 JSON 형식으로 모든 파일에 대한 결과를 응답하세요. 다른 설명은 절대 포함하지 마세요.
{{
  "results": [
    {{
      "file_path": "파일명1",
      "issues": [
        {{
          "line": 10,
          "vulnerability": "취약점명",
          "severity": "High",
          "description": "상세 설명",
          "original_code": "기존 코드",
          "suggested_code": "수정 후 코드",
          "action_type": "REPLACE"
        }}
      ]
    }},
    {{
      "file_path": "파일명2",
      "issues": []
    }}
  ]
}}
"""

async def perform_audit(filename: str, code: str) -> List[Issue]:
    # Read checklist
    checklist_path = r"c:\Users\dltkd\antigravity\보안취약점검사\보안성체크리스트.txt"
    if not os.path.exists(checklist_path):
         return [Issue(line=0, vulnerability="System Error", severity="High", description="체크리스트 파일을 찾을 수 없습니다.", original_code="", suggested_code="", action_type="ERROR")]

    with open(checklist_path, "r", encoding="utf-8") as f:
        checklist = f.read()

    max_retries = 3
    retry_delay = 5  # Start with 5 seconds

    for attempt in range(max_retries):
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
            print(f"Gemini API Error for {filename} (Attempt {attempt + 1}/{max_retries}): {api_err}")
            
            if "429" in str(api_err) and attempt < max_retries - 1:
                print(f"Rate limit hit. Retrying in {retry_delay} seconds...")
                time.sleep(retry_delay)
                retry_delay *= 2  # Exponential backoff
                continue
                
            if "429" in str(api_err):
                 return [Issue(
                    line=1,
                    vulnerability="API Quota Exceeded",
                    severity="Medium",
                    description="Gemini API 쿼터(분당 요청 횟수 등)가 여러 번의 재시도 끝에 최종 초과되었습니다.",
                    original_code=code.split('\n')[0] if code else "source code",
                    suggested_code="https://aistudio.google.com/ 에서 쿼터를 확인하거나 잠시 뒤 다시 시도하세요. (무료 티어는 분당 요청 제한이 엄격합니다)",
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
    supported_extensions = ('.py', '.js', '.ts', '.tsx', '.java', '.jsp', '.html', '.css', '.c', '.cpp', '.cc', '.cxx', '.h', '.hpp', '.hxx', '.php')
    valid_files = [f for f in files if f.filename.lower().endswith(supported_extensions)]
    
    if not valid_files:
        return []

    # Read checklist
    checklist_path = r"c:\Users\dltkd\antigravity\보안취약점검사\보안성체크리스트.txt"
    checklist = ""
    if os.path.exists(checklist_path):
        with open(checklist_path, "r", encoding="utf-8") as f:
            checklist = f.read()

    # Prepare files content for a single prompt
    files_text_block = ""
    for file in valid_files:
        content = await file.read()
        code = content.decode("utf-8")
        files_text_block += f"\n--- FILE: {file.filename} ---\n{code}\n"

    try:
        model = genai.GenerativeModel("gemini-2.0-flash")
        prompt = BATCH_AUDIT_PROMPT.format(
            checklist=checklist,
            files_content=files_text_block
        )
        
        response = model.generate_content(prompt)
        resp_text = response.text.strip()
        if resp_text.startswith("```json"):
            resp_text = resp_text[7:-3].strip()
        elif resp_text.startswith("```"):
            resp_text = resp_text[3:-3].strip()
            
        batch_data = json.loads(resp_text)
        results = []
        for res in batch_data.get("results", []):
            results.append(AuditResult(
                file_path=res["file_path"],
                issues=[Issue(**issue) for issue in res["issues"]]
            ))
        
        # Ensure all requested files are in the result even if AI missed them
        received_paths = {r.file_path for r in results}
        for f in valid_files:
            if f.filename not in received_paths:
                results.append(AuditResult(file_path=f.filename, issues=[]))
                
        return results
        
    except Exception as e:
        print(f"Batch Audit Error: {e}")
        # Fallback: if batch fails, return empty issues for all files with the error
        return [AuditResult(file_path=f.filename, issues=[
            Issue(line=1, vulnerability="Batch Audit Error", severity="Low", 
                  description=f"배치 분석 중 오류가 발생했습니다: {str(e)}", 
                  original_code="", suggested_code="", action_type="INFO")
        ]) for f in valid_files]

@app.get("/api/checklist")
async def get_checklist():
    checklist_path = r"c:\Users\dltkd\antigravity\보안취약점검사\보안성체크리스트.txt"
    if not os.path.exists(checklist_path):
        return {"text": ""}
    
    with open(checklist_path, "r", encoding="utf-8") as f:
        content = f.read()
    return {"text": content}

@app.post("/api/checklist")
async def update_checklist(update: ChecklistUpdate):
    checklist_path = r"c:\Users\dltkd\antigravity\보안취약점검사\보안성체크리스트.txt"
    try:
        with open(checklist_path, "w", encoding="utf-8") as f:
            f.write(update.text)
        return {"status": "success"}
    except Exception as e:
        raise HTTPException(status_code=500, detail=str(e))

if __name__ == "__main__":
    uvicorn.run(app, host="0.0.0.0", port=8001)
