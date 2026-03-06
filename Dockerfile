# --- Stage 1: Build Frontend ---
FROM node:20-slim AS build-stage
WORKDIR /app/web

# 패키지 매니저 파일복사 및 의존성 설치
COPY web/package*.json ./
RUN npm install

# 소스코드 전체 복사 및 빌드
COPY web/ ./
RUN npm run build

# --- Stage 2: Runtime Stage ---
FROM python:3.9-slim
WORKDIR /app

# 백엔드 의존성 설치
COPY requirements.txt .
RUN pip install --no-cache-dir -r requirements.txt

# 빌드물 및 백엔드 코드 복사
COPY . .
# 1단계에서 빌드된 dist 폴더를 복사하여 백엔드가 서빙 가능하도록 함
COPY --from=build-stage /app/web/dist /app/web/dist

# Cloud Run은 PORT 환경 변수를 사용함 (기본 8080)
ENV PORT 8080

# 서버 실행
CMD ["python", "audit_backend.py"]
