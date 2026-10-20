import { Injectable } from '@angular/core';

@Injectable(
    {providedIn: 'root'}
)

export class Counter {
    count: number = 0;

    add(): void {
        this.count++;
    }

    subtract(): void {
        this.count--;
    }
}
