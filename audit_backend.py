import os
import json
import uvicorn
import time
import shutil
import glob
from fastapi import FastAPI, UploadFile, File, HTTPException, Query
from fastapi.middleware.cors import CORSMiddleware
import google.generativeai as genai
from pydantic import BaseModel
from typing import List, Optional
from dotenv import load_dotenv

# Load environment variables from .env file, overriding existing ones
load_dotenv(override=True)

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

class ChecklistItem(BaseModel):
    name: str # e.g. "Security.txt"
    text: str

class AuditBatchRequest(BaseModel):
    selected_checklists: List[str] # List of filenames

def safe_apply_fix(file_path: str, original: str, suggested: str) -> bool:
    """
    Finds the correct file path (handling selective _fixed copy),
    copies it if necessary, and applies the fix.
    """
    if ".." in file_path:
        return False
        
    current_dir = os.path.abspath(os.getcwd())
    src_abs_path = os.path.abspath(file_path)
    
    # 1. Determine the source and destination paths
    # If we find a '_fixed' suffix in any part of the path, we might already be in a fixed directory.
    # Otherwise, we map 'Project' -> 'Project_fixed'
    
    parts = src_abs_path.replace("\\", "/").split("/")
    target_abs_path = None
    
    # Search for the base directory that we copied
    # Logic: If '보안취약점검사' is in the path, it should be '보안취약점검사_fixed'
    # We find which part of the path is our current working directory
    workspace_name = os.path.basename(current_dir)
    
    if workspace_name in parts:
        # Create the fixed path by replacing workspace_name with workspace_name_fixed
        fixed_parts = []
        replaced = False
        for p in parts:
            if not replaced and p == workspace_name:
                fixed_parts.append(p + "_fixed")
                replaced = True
            else:
                fixed_parts.append(p)
        
        target_abs_path = os.path.sep.join(fixed_parts)
    else:
        # fallback for relative paths or different structures
        target_abs_path = src_abs_path

    # 2. Ensure the destination file exists (Selective Copy)
    if workspace_name + "_fixed" in target_abs_path:
        if not os.path.exists(target_abs_path):
            if os.path.exists(src_abs_path):
                try:
                    os.makedirs(os.path.dirname(target_abs_path), exist_ok=True)
                    import shutil
                    shutil.copy2(src_abs_path, target_abs_path)
                    print(f"DEBUG: Selective Copy Applied: {src_abs_path} -> {target_abs_path}")
                except Exception as e:
                    print(f"Error copying file for selective fix: {e}")
                    return False
            else:
                print(f"Source file not found for copy: {src_abs_path}")
                return False

    # 3. Apply the fix to the target file
    return apply_fix_internal(target_abs_path, original, suggested)

def apply_fix_internal(target_full_path: str, original: str, suggested: str) -> bool:
    """
    Actual file write operation.
    """
    if not target_full_path or not os.path.exists(target_full_path):
        print(f"Target file not found for write: {target_full_path}")
        return False

    try:
        # We need to handle encodings here as well
        with open(target_full_path, "rb") as f:
            raw_content = f.read()
        
        content = decode_content(raw_content)
        
        # Exact match check
        if original in content:
            new_content = content.replace(original, suggested)
            with open(target_full_path, "w", encoding="utf-8") as f:
                f.write(new_content)
            print(f"Successfully applied fix to: {target_full_path}")
            return True
        else:
            if suggested in content:
                print(f"Fix already applied in: {target_full_path}")
                return True
            print(f"Original content mismatch in {target_full_path}")
            return False
    except Exception as e:
        print(f"Error writing fix to {target_full_path}: {e}")
        return False

def decode_content(content: bytes) -> str:
    """
    Attempts to decode bytes using UTF-8, then CP949 if it fails.
    """
    try:
        return content.decode("utf-8")
    except UnicodeDecodeError:
        try:
            return content.decode("cp949")
        except UnicodeDecodeError:
            return content.decode("utf-8", errors="ignore")

import shutil

# Prompt template for Single File Audit
AUDIT_PROMPT = """
당신은 전문 보안 시큐어 코딩 및 개인정보보호 감사관입니다.
다음 제공된 '체크리스트 가이드'의 규칙을 최우선으로 적용하여, 입력된 소스 코드에서 보안 취약점과 개인정보 노출 위험을 찾아내세요.
모든 분석 결과(취약점명, 상세 설명 등)는 반드시 한국어로 작성해야 합니다.

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
모든 분석 결과(취약점명, 상세 설명 등)는 반드시 한국어로 작성해야 합니다.

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

            # Call Gemini (Using stable 2.5 Flash as confirmed by ListModels)
            model = genai.GenerativeModel("gemini-2.5-flash")
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
        code = decode_content(content)
        issues = await perform_audit(file.filename, code)
        return {"file_path": file.filename, "issues": issues}
    except Exception as e:
        print(f"Failed to audit {file.filename}: {e}")
        raise HTTPException(status_code=500, detail=str(e))

@app.post("/api/audit-batch", response_model=List[AuditResult])
async def audit_batch(
    files: List[UploadFile] = File(...),
    checklists: List[str] = Query([]) # Selected checklist names
):
    supported_extensions = ('.py', '.js', '.ts', '.tsx', '.java', '.jsp', '.html', '.css', '.c', '.cpp', '.cc', '.cxx', '.h', '.hpp', '.hxx', '.php')
    valid_files = [f for f in files if f.filename.lower().endswith(supported_extensions)]
    
    if not valid_files:
        return []

    # Read selected checklists
    current_dir = os.path.dirname(os.path.abspath(__file__))
    checklist_dir = os.path.join(current_dir, "checklists")
    combined_checklist = ""
    
    # Auto-migration and directory check
    if not os.path.exists(checklist_dir):
        os.makedirs(checklist_dir, exist_ok=True)
        old_file = os.path.join(current_dir, "보안성체크리스트.txt")
        if os.path.exists(old_file):
            shutil.copy2(old_file, os.path.join(checklist_dir, "Default.txt"))

    if not checklists:
        # If none selected, try to use Default.txt as fallback
        fallback = os.path.join(checklist_dir, "Default.txt")
        if os.path.exists(fallback):
            with open(fallback, "r", encoding="utf-8") as f:
                combined_checklist = f.read()
    else:
        for c_name in checklists:
            c_path = os.path.join(checklist_dir, c_name)
            if os.path.exists(c_path):
                with open(c_path, "r", encoding="utf-8") as f:
                    combined_checklist += f"\n--- CHECKLIST: {c_name} ---\n"
                    combined_checklist += f.read() + "\n"

    # Prepare files content for a single prompt
    files_text_block = ""
    for file in valid_files:
        content = await file.read()
        code = decode_content(content)
        files_text_block += f"\n--- FILE: {file.filename} ---\n{code}\n"

    if not GOOGLE_API_KEY:
        return [AuditResult(file_path=f.filename, issues=[
            Issue(line=1, vulnerability="API Configuration Missing", severity="High", 
                  description="GOOGLE_API_KEY가 설정되지 않았습니다. 프로젝트 루트의 .env 파일에 키를 설정해주세요.", 
                  original_code="config error", suggested_code="GOOGLE_API_KEY=YOUR_KEY_HERE", action_type="INFO")
        ]) for f in valid_files]

    max_retries = 3
    retry_delay = 10  # Start with 10 seconds for batch

    for attempt in range(max_retries):
        try:
            model = genai.GenerativeModel("gemini-2.5-flash")
            prompt = BATCH_AUDIT_PROMPT.format(
                checklist=combined_checklist,
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
            print(f"Batch Audit Error (Attempt {attempt + 1}/{max_retries}): {e}")
            
            if "429" in str(e) and attempt < max_retries - 1:
                print(f"Rate limit hit in batch. Retrying in {retry_delay} seconds...")
                time.sleep(retry_delay)
                retry_delay *= 2
                continue

            # Final fall through or error return
            error_msg = str(e)
            if "429" in error_msg:
                error_msg = "Gemini API의 분당 요청 제한(RPM) 또는 토큰 제한(TPM)을 초과했습니다. 잠시 후 상단 '새로고침' 또는 '검사 시작'을 다시 눌러주세요."

            return [AuditResult(file_path=f.filename, issues=[
                Issue(line=1, vulnerability="Batch Audit Error", severity="Low", 
                    description=f"배치 분석 중 오류가 발생했습니다: {error_msg}", 
                    original_code="", suggested_code="API 쿼터 설정을 확인하거나 잠시 대기 후 시도하세요.", action_type="INFO")
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
    current_dir_abs = os.path.abspath(current_dir)
    current_dir_name = os.path.basename(current_dir_abs)
    
    # Normalize path separators
    raw_base_path = request.base_path.replace("\\", "/")
    path_parts = raw_base_path.split("/")
    base_path_from_request = path_parts[0] if path_parts else ""
    
    print(f"DEBUG: create_fix_copy request.base_path={request.base_path}")
    print(f"DEBUG: current_dir={current_dir}")
    print(f"DEBUG: current_dir_name={current_dir_name}, base_path_from_request={base_path_from_request}")

    if ".." in raw_base_path or not base_path_from_request:
        raise HTTPException(status_code=400, detail="Invalid base path.")

    # Case-insensitive comparison for Windows
    if base_path_from_request.lower() == current_dir_name.lower():
        src_path = current_dir_abs
    else:
        # Try joining with current_dir
        src_path = os.path.abspath(os.path.join(current_dir, base_path_from_request))
        
        # If not exists, maybe the path is relative to current_dir's parent? 
        # (Though usually it should be within the workspace)
        if not os.path.exists(src_path):
             print(f"DEBUG: src_path {src_path} not found. checking if it exists as is...")
             if os.path.exists(base_path_from_request):
                 src_path = os.path.abspath(base_path_from_request)

    src_path = os.path.normpath(src_path)
    print(f"DEBUG: Final src_path to copy: {src_path}")

    if not os.path.exists(src_path) or not os.path.isdir(src_path):
        print(f"ERROR: Source directory not found: {src_path}")
        raise HTTPException(status_code=404, detail=f"Source directory not found: {src_path}")

    dst_path = f"{src_path}_fixed"
    print(f"DEBUG: dst_path (directory only): {dst_path}")

    if not os.path.exists(dst_path):
        try:
            os.makedirs(dst_path, exist_ok=True)
            print(f"SUCCESS: Created fixed project root at {dst_path}")
            return {"status": "success", "message": "Created fixed project structure (Selective Copy mode).", "path": dst_path}
        except Exception as e:
            print(f"ERROR creating project directory: {e}")
            raise HTTPException(status_code=500, detail=f"Failed to create directory: {str(e)}")
    
    return {"status": "exists", "message": "Fixed version directory already exists.", "path": dst_path}

@app.get("/api/checklists")
async def list_checklists():
    current_dir = os.path.dirname(os.path.abspath(__file__))
    checklist_dir = os.path.join(current_dir, "checklists")
    if not os.path.exists(checklist_dir):
        os.makedirs(checklist_dir, exist_ok=True)
        # Migration
        old_file = os.path.join(current_dir, "보안성체크리스트.txt")
        if os.path.exists(old_file):
            shutil.copy2(old_file, os.path.join(checklist_dir, "Default.txt"))
            
    files = glob.glob(os.path.join(checklist_dir, "*.txt"))
    return [os.path.basename(f) for f in files]

@app.get("/api/checklists/{filename}")
async def get_checklist(filename: str):
    current_dir = os.path.dirname(os.path.abspath(__file__))
    path = os.path.join(current_dir, "checklists", filename)
    if not os.path.exists(path):
        raise HTTPException(status_code=404, detail="Checklist not found")
    with open(path, "r", encoding="utf-8") as f:
        return {"name": filename, "text": f.read()}

@app.post("/api/checklists")
async def save_checklist(item: ChecklistItem):
    current_dir = os.path.dirname(os.path.abspath(__file__))
    checklist_dir = os.path.join(current_dir, "checklists")
    os.makedirs(checklist_dir, exist_ok=True)
    
    filename = item.name
    if not filename.endswith(".txt"):
        filename += ".txt"
        
    path = os.path.join(checklist_dir, filename)
    with open(path, "w", encoding="utf-8") as f:
        f.write(item.text)
    return {"status": "success"}

@app.delete("/api/checklists/{filename}")
async def delete_checklist(filename: str):
    current_dir = os.path.dirname(os.path.abspath(__file__))
    path = os.path.join(current_dir, "checklists", filename)
    if os.path.exists(path):
        os.remove(path)
        return {"status": "success"}
    raise HTTPException(status_code=404, detail="Checklist not found")

if __name__ == "__main__":
    print("Starting server on http://0.0.0.0:8005")
    uvicorn.run(app, host="0.0.0.0", port=8005)
