import base64
import re
from datetime import datetime
from zoneinfo import ZoneInfo
from flask import Flask, request, jsonify
from google.cloud import vision
from google.oauth2 import service_account
from googleapiclient.discovery import build
from googleapiclient.http import MediaInMemoryUpload

app = Flask(__name__)

# === CONFIGURATION === #

# Path to your service account JSON key file
SERVICE_ACCOUNT_FILE = 'vision_key.json'

# Google Sheet ID
SPREADSHEET_ID = '1QU5aeHdGRGocN4htIKhT7geIgayR7qrSYzSed3akHvA'

# Drive main folder name
DRIVE_MAIN_FOLDER_NAME = 'ESP32-CAM'

# === INIT CLIENTS === #

creds = service_account.Credentials.from_service_account_file(
    SERVICE_ACCOUNT_FILE,
    scopes=[
        'https://www.googleapis.com/auth/spreadsheets',
        'https://www.googleapis.com/auth/drive',
        'https://www.googleapis.com/auth/cloud-platform'
    ]
)

# Vision API client
vision_client = vision.ImageAnnotatorClient(credentials=creds)

# Sheets API client
sheets_service = build('sheets', 'v4', credentials=creds)

# Drive API client
drive_service = build('drive', 'v3', credentials=creds)

# === UTILITY FUNCTIONS === #

def extract_number_plate(full_text):
    # Regex for Indian plates: MH 20 EE 7602, KA03MN7654, etc.
    pattern = r'([A-Z]{2}\s?\d{1,2}\s?[A-Z]{0,2}\s?\d{4})'
    match = re.search(pattern, full_text.replace('\n', ' '))
    return match.group(1) if match else "No plate number found"

def get_or_create_main_folder():
    query = f"mimeType='application/vnd.google-apps.folder' and name='{DRIVE_MAIN_FOLDER_NAME}' and trashed=false"
    results = drive_service.files().list(q=query, fields="files(id, name)").execute()
    files = results.get('files', [])
    if files:
        return files[0]['id']
    else:
        file_metadata = {
            'name': DRIVE_MAIN_FOLDER_NAME,
            'mimeType': 'application/vnd.google-apps.folder'
        }
        folder = drive_service.files().create(body=file_metadata, fields='id').execute()
        return folder.get('id')

def get_or_create_date_folder(date_str, parent_folder_id):
    query = f"mimeType='application/vnd.google-apps.folder' and name='{date_str}' and '{parent_folder_id}' in parents and trashed=false"
    results = drive_service.files().list(q=query, fields="files(id, name)").execute()
    files = results.get('files', [])
    if files:
        return files[0]['id']
    else:
        file_metadata = {
            'name': date_str,
            'mimeType': 'application/vnd.google-apps.folder',
            'parents': [parent_folder_id]
        }
        folder = drive_service.files().create(body=file_metadata, fields='id').execute()
        return folder.get('id')

def upload_image_to_drive(image_bytes, image_name, date_folder_id):
    media = MediaInMemoryUpload(image_bytes, mimetype='image/jpeg')
    file_metadata = {
        'name': image_name,
        'parents': [date_folder_id]
    }
    file = drive_service.files().create(body=file_metadata, media_body=media, fields='id').execute()
    drive_service.permissions().create(
        fileId=file['id'],
        body={'role': 'reader', 'type': 'anyone'}
    ).execute()
    file_id = file.get('id')
    return f"https://drive.google.com/uc?id={file_id}&export=view"

# === FLASK ROUTE === #

@app.route('/', methods=['POST'])
def process_image():
    try:
        data = request.get_json()
        base64_image = data['image']
        ist_tz = ZoneInfo("Asia/Kolkata")
        now_ist = datetime.now(ist_tz)
        image_name = data.get('image_name', f'esp32cam_{now_ist.strftime("%H%M%S")}.jpg')

        # Decode the base64 image
        image_bytes = base64.b64decode(base64_image)
        image = vision.Image(content=image_bytes)

        # Detect text using Google Vision
        response = vision_client.text_detection(image=image)
        detected_text = "No text detected"
        if response.text_annotations:
            full_text = response.text_annotations[0].description.strip()
            detected_text = extract_number_plate(full_text)

        # Upload image to Drive
        date_str = now_ist.strftime("%Y-%m-%d")
        main_folder_id = get_or_create_main_folder()
        date_folder_id = get_or_create_date_folder(date_str, main_folder_id)
        drive_url = upload_image_to_drive(image_bytes, image_name, date_folder_id)

        # Append the data to Google Sheet (date and time in separate columns)
        date = now_ist.strftime("%Y-%m-%d")
        time = now_ist.strftime("%H:%M:%S")
        values = [[date, time, drive_url, detected_text]]
        sheets_service.spreadsheets().values().append(
            spreadsheetId=SPREADSHEET_ID,
            range='Sheet1!A:D',
            valueInputOption='USER_ENTERED',
            body={'values': values}
        ).execute()

        # Return detected text and drive URL
        return jsonify({
            "detected_text": detected_text,
            "drive_url": drive_url
        })

    except Exception as e:
        import traceback
        print(f"Error in process_image: {str(e)}")
        traceback.print_exc()
        return jsonify({"error": str(e)}), 500

if __name__ == '__main__':
    import os
    port = int(os.environ.get('PORT', 8080))
    app.run(host='0.0.0.0', port=port, debug=False)