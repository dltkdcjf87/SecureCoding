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
@app.get("/")
async def root():
    return {"status": "ok", "message": "Security Audit API is running"}

@app.get("/api/health")
async def health():
    return {"status": "ok", "api_key_set": GOOGLE_API_KEY is not None}


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
    print("CRITICAL: GOOGLE_API_KEY environment variable is not set.")
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

class FixRequest(BaseModel):
    file_path: str
    original_code: str
    suggested_code: str

class BatchFixRequest(BaseModel):
    fixes: List[FixRequest]

class CreateCopyRequest(BaseModel):
    base_path: str

def safe_apply_fix(file_path: str, original: str, suggested: str) -> bool:
    """
    Attempts to find the file and replace the original code with suggested code.
    Prioritizes the '_fixed' directory if it exists.
    """
    current_dir = os.getcwd()
    
    # Identify if a fixed version of the base directory already exists
    # If the file_path starts with 'test_cases/', we should check if 'test_cases_fixed/' exists.
    path_parts = file_path.replace("\\", "/").split("/")
    base_folder = path_parts[0] if len(path_parts) > 1 else ""
    
    fixed_base_folder = f"{base_folder}_fixed" if base_folder else ""
    fixed_file_path = file_path.replace(base_folder, fixed_base_folder, 1) if fixed_base_folder else file_path

    potential_paths = [
        os.path.abspath(os.path.join(current_dir, fixed_file_path)),
        os.path.abspath(os.path.join(current_dir, file_path)),
        os.path.abspath(fixed_file_path),
        os.path.abspath(file_path)
    ]
    
    if ".." in file_path:
        return False

    target_full_path = None
    for p in potential_paths:
        if os.path.exists(p) and os.path.isfile(p):
            target_full_path = p
            break
    
    # Fallback to recursively searching in _fixed first, then original
    if not target_full_path:
        base_name = os.path.basename(file_path)
        # Search in _fixed directories first
        for root, dirs, files in os.walk(current_dir):
            if "_fixed" in root and base_name in files:
                target_full_path = os.path.join(root, base_name)
                break
        
        if not target_full_path:
            for root, dirs, files in os.walk(current_dir):
                if ".git" in root or "node_modules" in root or "_fixed" in root: continue
                if base_name in files:
                    target_full_path = os.path.join(root, base_name)
                    break

    if not target_full_path:
        print(f"File not found for fix: {file_path}")
        return False

    try:
        with open(target_full_path, "r", encoding="utf-8") as f:
            content = f.read()
        
        # Exact match check
        if original in content:
            new_content = content.replace(original, suggested)
            with open(target_full_path, "w", encoding="utf-8") as f:
                f.write(new_content)
            print(f"Successfully applied fix to: {target_full_path}")
            return True
        else:
            # Fallback: check if content already matches suggested (already applied)
            if suggested in content:
                print(f"Fix already applied or exists in: {target_full_path}")
                return True
            print(f"Original content mismatch in {target_full_path}")
            return False
    except Exception as e:
        print(f"Error applying fix to {target_full_path}: {e}")
        return False

import shutil

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
    current_dir = os.path.dirname(os.path.abspath(__file__))
    checklist_path = os.path.join(current_dir, "보안성체크리스트.txt")
    if not os.path.exists(checklist_path):
         return [Issue(line=0, vulnerability="System Error", severity="High", description=f"체크리스트 파일을 찾을 수 없습니다. (경로: {checklist_path})", original_code="", suggested_code="", action_type="ERROR")]

    with open(checklist_path, "r", encoding="utf-8") as f:
        checklist = f.read()

    max_retries = 3
    retry_delay = 5  # Start with 5 seconds

    for attempt in range(max_retries):
        try:
            if not GOOGLE_API_KEY:
                return [Issue(
                    line=1,
                    vulnerability="API Configuration Missing",
                    severity="High",
                    description="GOOGLE_API_KEY가 설정되지 않았습니다. 프로젝트 루트의 .env 파일에 키를 설정해주세요.",
                    original_code="config error",
                    suggested_code="GOOGLE_API_KEY=YOUR_KEY_HERE",
                    action_type="INFO"
                )]

            # Call Gemini
            model = genai.GenerativeModel("gemini-flash-latest")
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
    current_dir = os.path.dirname(os.path.abspath(__file__))
    checklist_path = os.path.join(current_dir, "보안성체크리스트.txt")
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
        if not GOOGLE_API_KEY:
            return [AuditResult(file_path=f.filename, issues=[
                Issue(line=1, vulnerability="API Configuration Missing", severity="High", 
                      description="GOOGLE_API_KEY가 설정되지 않았습니다. 프로젝트 루트의 .env 파일에 키를 설정해주세요.", 
                      original_code="config error", suggested_code="GOOGLE_API_KEY=YOUR_KEY_HERE", action_type="INFO")
            ]) for f in valid_files]

        model = genai.GenerativeModel("gemini-flash-latest")
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

@app.post("/api/apply-fix")
async def apply_fix(fix: FixRequest):
    success = safe_apply_fix(fix.file_path, fix.original_code, fix.suggested_code)
    if not success:
        raise HTTPException(status_code=400, detail="Failed to apply fix. File not found or code mismatch.")
    return {"status": "success"}

@app.post("/api/apply-fix-batch")
async def apply_fix_batch(batch: BatchFixRequest):
    results = []
    for fix in batch.fixes:
        success = safe_apply_fix(fix.file_path, fix.original_code, fix.suggested_code)
        results.append({"file_path": fix.file_path, "success": success})
    
    return {"status": "success", "results": results}

@app.post("/api/create-fix-copy")
async def create_fix_copy(request: CreateCopyRequest):
    """
    Creates a copy of the specified directory with a '_fixed' suffix.
    """
    current_dir = os.getcwd()
    base_path = request.base_path.replace("\\", "/").split("/")[0] if "/" in request.base_path or "\\" in request.base_path else request.base_path
    
    if ".." in base_path or not base_path:
        raise HTTPException(status_code=400, detail="Invalid base path.")

    src_path = os.path.abspath(os.path.join(current_dir, base_path))
    dst_path = f"{src_path}_fixed"

    if os.path.exists(dst_path):
        return {"status": "exists", "message": "Fixed version already exists.", "path": dst_path}

    try:
        shutil.copytree(src_path, dst_path)
        return {"status": "success", "message": "Created fixed project copy.", "path": dst_path}
    except Exception as e:
        print(f"Error creating project copy: {e}")
        raise HTTPException(status_code=500, detail=f"Failed to create copy: {str(e)}")

@app.get("/api/checklist")
async def get_checklist():
    current_dir = os.path.dirname(os.path.abspath(__file__))
    checklist_path = os.path.join(current_dir, "보안성체크리스트.txt")
    if not os.path.exists(checklist_path):
        return {"text": ""}
    
    with open(checklist_path, "r", encoding="utf-8") as f:
        content = f.read()
    return {"text": content}

@app.post("/api/checklist")
async def update_checklist(update: ChecklistUpdate):
    current_dir = os.path.dirname(os.path.abspath(__file__))
    checklist_path = os.path.join(current_dir, "보안성체크리스트.txt")
    try:
        with open(checklist_path, "w", encoding="utf-8") as f:
            f.write(update.text)
        return {"status": "success"}
    except Exception as e:
        raise HTTPException(status_code=500, detail=str(e))

if __name__ == "__main__":
    print("Starting server on http://0.0.0.0:8005")
    uvicorn.run(app, host="0.0.0.0", port=8005)
