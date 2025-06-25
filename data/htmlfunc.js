// перепрігивание по полям ip при наборе 3х цифр
function moveNext(input, event) {
    if (input.value.length >= 3) {
        const next = input.nextElementSibling?.nextElementSibling;
        if (next && next.tagName === 'INPUT') {
            next.focus();
        }
    }
}


function confirmDelete(event) {
    event.preventDefault(); // Останавливаем переход по ссылке
    const confirmed = confirm("Ви впевнені, що хочете видалити log.txt?");
    if (confirmed) {
        window.location.href = "/deletelog?deletelog=true";
    }
}



