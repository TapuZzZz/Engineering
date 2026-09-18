import { Component, Input } from '@angular/core';
import { CommonModule } from '@angular/common';
import { Hobby } from '../../models/hobby';

@Component({
  selector: 'app-cards',
  imports: [CommonModule],
  templateUrl: './cards.html',
  styleUrl: './cards.css',
})
export class Cards {
  @Input() hobbies: Hobby[] = [];
}