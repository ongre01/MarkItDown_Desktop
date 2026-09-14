#include "ConversionErrorPresentation.h"

#include <QCoreApplication>

ConversionErrorPresentation conversionErrorPresentation(ConversionError error)
{
    switch (error) {
    case ConversionError::AlreadyRunning:
        return {QCoreApplication::translate("MainWindow", "Conversion Already Running"),
                QCoreApplication::translate(
                    "MainWindow", u8"다른 문서 변환이 이미 실행 중입니다.")};
    case ConversionError::ExecutableNotFound:
        return {QCoreApplication::translate("MainWindow", "MarkItDown Not Found"),
                QCoreApplication::translate(
                    "MainWindow",
                    u8"MarkItDown 실행 파일을 찾을 수 없습니다. 앱 로컬 백엔드 설치 또는 "
                    u8"MARKITDOWN_EXECUTABLE 설정을 확인하세요.")};
    case ConversionError::FailedToStart:
        return {QCoreApplication::translate("MainWindow", "MarkItDown Start Failed"),
                QCoreApplication::translate(
                    "MainWindow", u8"MarkItDown 프로세스를 시작하지 못했습니다.")};
    case ConversionError::Crashed:
        return {QCoreApplication::translate("MainWindow", "MarkItDown Crashed"),
                QCoreApplication::translate(
                    "MainWindow", u8"변환 중 MarkItDown 프로세스가 비정상 종료되었습니다.")};
    case ConversionError::NonZeroExit:
        return {QCoreApplication::translate("MainWindow", "Conversion Command Failed"),
                QCoreApplication::translate(
                    "MainWindow", u8"MarkItDown 변환 명령이 오류 종료 코드를 반환했습니다.")};
    case ConversionError::EmptyOutput:
        return {QCoreApplication::translate("MainWindow", "Empty Conversion Output"),
                QCoreApplication::translate(
                    "MainWindow", u8"MarkItDown 변환 결과가 비어 있습니다.")};
    case ConversionError::ProcessFailure:
        return {QCoreApplication::translate("MainWindow", "MarkItDown Execution Failed"),
                QCoreApplication::translate(
                    "MainWindow", u8"MarkItDown 프로세스 실행 중 오류가 발생했습니다.")};
    }

    return {QCoreApplication::translate("MainWindow", "Conversion Failed"),
            QCoreApplication::translate("MainWindow", u8"문서 변환에 실패했습니다.")};
}
