import os

def convert_files_to_utf8(directory):
    extensions = ('.h', '.cpp', '.cs', '.Build.cs', '.Target.cs')
    converted_count = 0
    
    for root, dirs, files in os.walk(directory):
        for file in files:
            if file.endswith(extensions):
                file_path = os.path.join(root, file)
                content = None
                encoding_used = None
                
                # Перебираем возможные кодировки, в которых могут быть файлы
                for enc in ['utf-16', 'utf-16-le', 'utf-16-be', 'utf-8-sig', 'cp1251', 'utf-8']:
                    try:
                        with open(file_path, 'r', encoding=enc) as f:
                            content = f.read()
                            encoding_used = enc
                            break
                    except (UnicodeDecodeError, LookupError):
                        continue
                
                if content is not None:
                    try:
                        # Перезаписываем файл строго в UTF-8 без BOM
                        with open(file_path, 'w', encoding='utf-8') as f:
                            f.write(content)
                        print(f"[OK] Конвертирован ({encoding_used} -> utf-8): {file_path}")
                        converted_count += 1
                    except Exception as e:
                        print(f"[шибка записи] {file_path}: {e}")
                else:
                    print(f"[ОШИБКА] Не удалось определить кодировку: {file_path}")

    print(f"\nГотово! Всего успешно конвертировано файлов: {converted_count}")

if __name__ == "__main__":
    target_dir = "./Source"
    if os.path.exists(target_dir):
        print(f"сканирование директории {target_dir}...")
        convert_files_to_utf8(target_dir)
    else:
        print(f"Ошибка: Директория {target_dir} не найдена в текущей папке проекта.")